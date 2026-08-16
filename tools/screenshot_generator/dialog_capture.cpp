#include "dialog_capture.h"

#include "null_task_manager.h"
#include "screenshot_config.h"
#include "screenshot_output.h"
#include "screenshot_wait.h"
#include "widget_capture.h"

#include "aui/qt/dialog_service_impl_qt.h"
#include "aui/translation.h"
#include "base/any_executor.h"
#include "base/boost_log.h"
#include "base/memory_settings_store.h"
#include "base/relative_time_range.h"
#include "base/utf_convert.h"
#include "controller/command_manager.h"
#include "main_window/command_palette_qt.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "modules/about/about_dialog.h"
#include "modules/change_password/change_password_dialog.h"
#include "modules/create_service_item/create_service_item_dialog.h"
#include "modules/events/local_events.h"
#include "modules/limits/limit_dialog.h"
#include "modules/login/login_dialog.h"
#include "modules/multi_create/multi_create_dialog.h"
#include "modules/time_range/time_range_dialog.h"
#include "modules/write/write_dialog.h"
#include "modules/write/write_model.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "profile/profile.h"
#include "scada/co_result.h"
#include "scada/data_services_factory.h"
#include "scada/logging.h"
#include "scada/node_id.h"
#include "services/task_manager.h"
#include "timed_data/timed_data_service.h"
#include "timed_data/timed_data_spec.h"
#include "ui/common/client_utils.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QElapsedTimer>
#include <QListWidget>
#include <QPainter>
#include <QPixmap>
#include <QString>
#include <QWidget>
#include <transport/transport_factory.h>

#include <chrono>
#include <gtest/gtest.h>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace {

using scada::screenshot_generator::WaitForPendingNodeLoads;

// Pumps the Qt event loop until `predicate()` holds or `timeout` elapses.
//
// Uses the single-argument processEvents(WaitForMoreEvents) form on purpose:
// the two-argument processEvents(flags, ms) overload strips
// QEventLoop::WaitForMoreEvents and returns as soon as the queue drains, and
// on the macOS (Cocoa) dispatcher a drained no-wait pass never activates
// QTimer timers — which starves MessageLoopQt's 10 ms task-queue timer, so
// CoSpawn'd dialog coroutines never get their initial resume. The
// single-argument form blocks until the next event (the 10 ms timer at the
// latest), so the executor's tasks actually run.
template <class Predicate>
bool PumpEventsUntil(Predicate&& predicate, std::chrono::milliseconds timeout) {
  QElapsedTimer timer;
  timer.start();
  while (!predicate()) {
    if (timer.elapsed() >= timeout.count())
      return false;
    QApplication::processEvents(QEventLoop::WaitForMoreEvents);
  }
  return true;
}

// Pumps the Qt event loop for a fixed duration, letting queued executor
// tasks and async value updates land.
void PumpEventsFor(std::chrono::milliseconds duration) {
  PumpEventsUntil([] { return false; }, duration);
}

template <class T>
struct DialogAwaitableResult {
  std::optional<T> value;
  std::exception_ptr error;
  bool done = false;
};

template <>
struct DialogAwaitableResult<void> {
  std::exception_ptr error;
  bool done = false;
};

template <class T>
std::shared_ptr<DialogAwaitableResult<T>> StartDialogAwaitable(
    AnyExecutor executor,
    Awaitable<T> awaitable) {
  auto result = std::make_shared<DialogAwaitableResult<T>>();
  CoSpawn(
      std::move(executor),
      [result, awaitable = std::move(awaitable)]() mutable -> Awaitable<void> {
        try {
          if constexpr (std::is_void_v<T>) {
            co_await std::move(awaitable);
          } else {
            result->value.emplace(co_await std::move(awaitable));
          }
        } catch (...) {
          result->error = std::current_exception();
        }
        result->done = true;
      });
  return result;
}

template <class T>
bool IsDialogAwaitableReady(
    const std::shared_ptr<DialogAwaitableResult<T>>& result) {
  return result->done;
}

template <class T>
void GetDialogAwaitableResult(
    const std::shared_ptr<DialogAwaitableResult<T>>& result) {
  if (result->error) {
    std::rethrow_exception(result->error);
  }
}

// Dummy TransportFactory that never produces a transport. LoginDialog
// & friends hold a `TransportFactory&` inside their DataServicesContext
// but only touch it on a real login attempt — we reject before that.
class NullTransportFactory : public transport::TransportFactory {
 public:
  transport::expected<transport::any_transport> CreateTransport(
      const transport::TransportString&,
      const transport::executor&,
      const transport::log_source&) override {
    return transport::ERR_NOT_IMPLEMENTED;
  }
};

// Makes the dialog's target node fully resident — attributes, property
// children, and the type-definition chain — so synchronous property reads
// (engineering units, limit bands, control flags) resolve when the dialog
// model is constructed.
bool FetchDialogNodeResident(NodeService& node_service,
                             const scada::NodeId& node_id) {
  return scada::screenshot_generator::FetchNodesResident(
      node_service, std::span<const scada::NodeId>{&node_id, 1});
}

// Grabs `dialog` with the drop-down of the combo box named `combo_object`
// open, composing the two into one pixmap.
//
// Qt gives a combo popup its own top-level window (`view()->window()`, a
// Qt::Popup frame), so `dialog->grab()` renders the widget tree with the list
// closed however the combo was opened — calling showPopup() before the grab
// is not enough, and that is why the login capture shipped a closed combo.
// The composed image is the union of the two windows' geometries, which means
// it is taller than the spec's `height` whenever the list hangs below the
// dialog, exactly as the manual's login image shows it.
QPixmap GrabDialogWithComboPopupOpen(QDialog* dialog,
                                     const std::string& combo_object) {
  auto* combo = dialog->findChild<QComboBox*>(
      QString::fromStdString(combo_object), Qt::FindChildrenRecursively);
  if (!combo) {
    ADD_FAILURE() << "expand_combo: no QComboBox named " << combo_object
                  << " in " << dialog->metaObject()->className();
    return GrabWhenSettled(dialog);
  }

  combo->showPopup();
  QWidget* popup = combo->view() ? combo->view()->window() : nullptr;
  PumpEventsUntil([popup] { return popup && popup->isVisible(); },
                  std::chrono::seconds{2});

  QPixmap dialog_pixmap = GrabWhenSettled(dialog);
  if (!popup || !popup->isVisible()) {
    ADD_FAILURE() << "expand_combo: " << combo_object
                  << " popup never became visible";
    return dialog_pixmap;
  }

  const QPixmap popup_pixmap = GrabWhenSettled(popup);
  const QRect dialog_rect{dialog->mapToGlobal(QPoint{0, 0}), dialog->size()};
  const QRect popup_rect{popup->mapToGlobal(QPoint{0, 0}), popup->size()};
  const QRect bounds = dialog_rect.united(popup_rect);

  const qreal ratio = dialog_pixmap.devicePixelRatio();
  QPixmap composed{QSize{static_cast<int>(bounds.width() * ratio),
                         static_cast<int>(bounds.height() * ratio)}};
  composed.setDevicePixelRatio(ratio);
  // The union can leave uncovered corners when the popup is wider than the
  // dialog (or vice versa); fill rather than leave them undefined.
  composed.fill(dialog->palette().color(dialog->backgroundRole()));
  {
    QPainter painter{&composed};
    painter.drawPixmap(dialog_rect.topLeft() - bounds.topLeft(), dialog_pixmap);
    painter.drawPixmap(popup_rect.topLeft() - bounds.topLeft(), popup_pixmap);
  }

  // Leave the combo closed: the popup is a top-level widget, and the next
  // capture's scan for a visible dialog walks that same list.
  combo->hidePopup();
  QApplication::processEvents();
  return composed;
}

// Scans top-level widgets for a visible QDialog, resizes it to the
// spec dims, grabs a pixmap, and rejects the dialog so whoever called
// `show()` can finish the cleanup path (deleteLater in most factories).
bool GrabAndCloseVisibleDialog(const DialogSpec& spec) {
  QDialog* dialog = nullptr;
  auto find_visible_dialog = [&dialog] {
    for (QWidget* w : QApplication::topLevelWidgets()) {
      if (w->isVisible()) {
        if (auto* d = qobject_cast<QDialog*>(w)) {
          dialog = d;
          return true;
        }
      }
    }
    return false;
  };
  // The dialog factories are coroutines CoSpawn'd onto MessageLoopQt; pump
  // (see PumpEventsUntil) until the initial resume shows the dialog.
  if (!PumpEventsUntil(find_visible_dialog, std::chrono::seconds{2}))
    return false;

  if (spec.width > 0 && spec.height > 0) {
    dialog->resize(spec.width, spec.height);
    QApplication::processEvents();
  }
  dialog->repaint();
  QApplication::processEvents();

  QPixmap pixmap = spec.expand_combo.empty() ? GrabWhenSettled(dialog)
                                             : GrabDialogWithComboPopupOpen(
                                                   dialog, spec.expand_combo);
  auto path = GetOutputDir() / spec.filename;
  pixmap.save(QString::fromStdString(path.string()));

  // Hide *before* reject(). LoginDialog overrides reject() to resolve
  // its completion and deliberately doesn't call QDialog::reject(), which
  // means reject() alone leaves the dialog visible — and the next
  // capture's top-level-widgets scan finds the login again instead of
  // the dialog just shown. An explicit hide() clears isVisible() for
  // that scan regardless of what reject() does.
  dialog->hide();
  dialog->reject();
  for (int i = 0; i < 3; ++i)
    QApplication::processEvents();
  return true;
}

bool GrabAndCloseVisibleDialogOrReport(const DialogSpec& spec) {
  if (GrabAndCloseVisibleDialog(spec))
    return true;
  std::ostringstream widgets;
  for (QWidget* w : QApplication::topLevelWidgets()) {
    widgets << " | " << w->metaObject()->className()
            << (w->isVisible() ? " (visible)" : " (hidden)");
  }
  ADD_FAILURE() << "No visible dialog for kind: " << spec.kind
                << " | top-level widgets:" << widgets.str();
  return false;
}

template <class T>
void WaitForDialogCompletion(
    const std::shared_ptr<DialogAwaitableResult<T>>& result) {
  if (!PumpEventsUntil([&] { return IsDialogAwaitableReady(result); },
                       std::chrono::seconds{4}))
    ADD_FAILURE() << "Dialog coroutine did not complete";

  try {
    GetDialogAwaitableResult(result);
  } catch (const std::exception&) {
    // Captures close dialogs through reject(), so modal-dialog awaitables
    // normally finish by throwing the cancellation exception.
  }
}

// --- Per-kind builders. Each invokes the component's public factory
// (which calls show() internally) and returns; the generic
// GrabAndCloseVisibleDialog then picks the dialog up from the
// top-level-widgets list.
//
// Adding a new dialog = add a new `Build*Dialog` here and a new arm in
// the `Dispatch` switch below.

std::shared_ptr<DialogAwaitableResult<std::optional<DataServices>>>
BuildLoginDialog(DialogEnvironment& env,
                 NullTransportFactory& transport_factory,
                 const std::shared_ptr<BoostLogger>& logger) {
  DataServicesContext services_context{logger, env.executor, transport_factory,
                                       scada::ServiceLogParams{}};
  // Hermetic settings: the production dialog reads the saved user list and
  // server addresses from the registry / per-user settings file, so on a
  // used dev box the capture would leak the real server address. Seed an
  // in-memory store with the state the docs image shows instead.
  //
  // The accounts come from the fixture's `login_user_list` and are joined the
  // way LoginController stores them — comma-separated, since it reads the
  // setting back through its own ParseListString.
  auto settings_store = std::make_shared<MemorySettingsStore>();
  std::u16string user_list;
  for (const auto& user : env.login_user_list) {
    if (!user_list.empty())
      user_list += u',';
    user_list += UtfConvert<char16_t>(user);
  }
  settings_store->SetString16(
      "User", env.login_user_list.empty()
                  ? std::u16string{}
                  : UtfConvert<char16_t>(env.login_user_list.front()));
  settings_store->SetString16("UserList", user_list);
  settings_store->SetString("Host", "127.0.0.1");
  auto dialog_lifetime = StartDialogAwaitable(
      env.executor,
      ExecuteLoginDialog(env.executor, std::move(services_context),
                         std::move(settings_store)));
  QApplication::processEvents();
  return dialog_lifetime;
}

// Limits dialog: needs a NodeRef to an analog variable plus a
// TaskManager for the (never-taken) write path. `node_id` is the spec's own
// target or, absent one, the fixture-wide dialog analog node; in the current
// fixture that is "Температура нагрева", the analog node the docs images
// target. The resident fetch below resolves property reads, so the four limit
// fields render from that node's limit_{lolo,lo,hi,hihi} properties — a node
// without them captures an empty dialog, which is how limits.png shipped
// blank.
std::shared_ptr<DialogAwaitableResult<void>> BuildLimitsDialog(
    DialogEnvironment& env,
    const scada::NodeId& node_id,
    NullTaskManager& task_manager,
    DialogServiceImplQt& dialog_service) {
  if (!env.node_service) {
    ADD_FAILURE() << "LimitsDialog needs a node_service in DialogEnvironment";
    return {};
  }
  auto node = env.node_service->GetNode(node_id);
  if (!node) {
    ADD_FAILURE() << "LimitsDialog: configured fixture node not found";
    return {};
  }
  if (!FetchDialogNodeResident(*env.node_service, node_id)) {
    ADD_FAILURE() << "LimitsDialog: failed to fetch configured fixture node";
    return {};
  }
  auto dialog_lifetime = StartDialogAwaitable(
      env.executor,
      ShowLimitsDialog(dialog_service,
                       LimitDialogContext{env.executor, node, task_manager}));
  QApplication::processEvents();
  return dialog_lifetime;
}

// Write dialog family. `manual` picks between "Manual Input" (TI
// manual override) and "Control" (remote device control). `node_id` comes
// from `screenshot_data.json` — the spec's own `node`, or the fixture-wide
// analog TI node the docs images use. Which of the two dialogs the model
// builds is the node's doing, not the kind's: WriteModel asks
// TimedDataSpec::logical(), so an AnalogItemType node renders the editable
// numeric field with its engineering unit (the ti-* captures) and a
// DiscreteItemType node renders the read-only two-state combo labelled from
// its HasTsFormat target (the ts-* captures, whose specs name TS.201/TS.202).
//
// After show() we pump events aggressively: the real TimedDataServiceImpl
// fulfils the current value through an async subscribe+read chain routed
// through TestExecutor and Qt's event loop, and the UI only updates via
// current_change_handler once the DataValue lands. Too few pumps leaves
// the "Current value:" label blank in the grab.
std::shared_ptr<DialogAwaitableResult<void>> BuildWriteDialog(
    DialogEnvironment& env,
    const scada::NodeId& node_id,
    DialogServiceImplQt& dialog_service,
    bool manual) {
  if (!env.timed_data_service || !env.profile || !env.node_service) {
    ADD_FAILURE() << "WriteDialog needs timed_data_service + profile + "
                     "node_service in env";
    return {};
  }
  auto node = env.node_service->GetNode(node_id);
  if (!node) {
    ADD_FAILURE() << "WriteDialog: configured fixture node not found";
    return {};
  }
  if (!FetchDialogNodeResident(*env.node_service, node_id)) {
    ADD_FAILURE() << "WriteDialog: failed to fetch configured fixture node";
    return {};
  }
  // Warm the item's current value before the dialog exists. WriteDialog reads
  // the discrete state once, in its constructor, and only ever refreshes the
  // "Current value:" label afterwards — so a value that lands later leaves the
  // combo showing GetCurrentDiscreteState()'s get_or(true) default beside a
  // label that has since corrected itself, and the dialog proposes the state
  // it is already in. Whether that happened used to depend on whether an
  // earlier capture in the same run had already subscribed to the same item:
  // ts-remote-control-enabled.png rendered correctly only because
  // ts-manual-control.png names the same node and ran first.
  TimedDataSpec warm_up{*env.timed_data_service, node_id};
  PumpEventsUntil([&warm_up] { return !warm_up.current().value.is_null(); },
                  std::chrono::seconds{2});

  WriteContext context{.executor_ = env.executor,
                       .timed_data_service_ = *env.timed_data_service,
                       .node_id_ = node_id,
                       .profile_ = *env.profile,
                       .manual_ = manual};
  auto dialog_lifetime = StartDialogAwaitable(
      env.executor, ExecuteWriteDialog(dialog_service, std::move(context)));
  PumpEventsFor(std::chrono::milliseconds{200});
  return dialog_lifetime;
}

// The operator's review of a control command. The prompt itself comes from a
// real WriteModel over a fixture item, so the capture cannot drift from the
// shipped wording; it is then shown through the ordinary DialogService
// question box, which is what StartWriting() does.
//
// `second_stage` picks which of the two prompts is rendered: the ordinary
// one-shot review, or the operate stage of a select-before-operate command,
// which GetConfirmationMessage prefixes with "the remote device is ready".
// The select is not actually issued in either case — a Control-object Call
// needs a server, and this capture is of the review step, not of the
// two-phase exchange (that is covered by the iec104 tier E2E and by
// WriteModelTest.TwoStagedControlSelectsConfirmsThenOperates).
std::shared_ptr<DialogAwaitableResult<MessageBoxResult>>
BuildControlConfirmation(DialogEnvironment& env,
                         const scada::NodeId& node_id,
                         DialogServiceImplQt& dialog_service,
                         bool second_stage,
                         std::optional<double> command_value) {
  if (!env.timed_data_service || !env.profile || !env.node_service) {
    ADD_FAILURE() << "Control confirmation needs timed_data_service + profile "
                     "+ node_service in env";
    return {};
  }
  if (!FetchDialogNodeResident(*env.node_service, node_id)) {
    ADD_FAILURE() << "Control confirmation: fixture node not found";
    return {};
  }
  // Warm the item's current value before the model exists, for the reason
  // BuildWriteDialog documents: the review quotes the present reading, and
  // WriteModel resolves it once. Without this the prompt shows
  // GetCurrentDiscreteState()'s get_or(true) default rather than the fixture
  // reading — and on a two-state item that default *is* one of the two
  // states, so the review renders a plausible no-op instead of failing.
  TimedDataSpec warm_up{*env.timed_data_service, node_id};
  PumpEventsUntil([&warm_up] { return !warm_up.current().value.is_null(); },
                  std::chrono::seconds{2});

  auto model = std::make_shared<WriteModel>(
      WriteContext{.executor_ = env.executor,
                   .timed_data_service_ = *env.timed_data_service,
                   .node_id_ = node_id,
                   .profile_ = *env.profile,
                   .manual_ = false});
  model->set_dialog_service(&dialog_service);

  // Default: a value clear of the analog fixture reading, so Present and
  // Command differ. A discrete item overrides it — its states are not on the
  // same scale — as does any capture wanting a particular commanded value.
  constexpr double kDefaultCommandedValue = 12.5;
  const double commanded = command_value.value_or(kDefaultCommandedValue);

  // The review exists so the operator can compare what the point reads now
  // against what the command makes it, so a capture whose two halves coincide
  // documents nothing. On a discrete item an out-of-range state is the
  // sharper version of the same fault: it has no label, so the prompt quotes
  // empty text and the capture still writes a well-formed PNG that
  // `check_screenshots.py` accepts — the `limits.png` failure mode again.
  if (model->discrete()) {
    // `commanded` is the item's raw value, which is not an index into
    // GetDiscreteStates(): WriteModel labels a state by inverting the raw
    // value (`get_or(true) ? 0 : 1`), so raw 0 is the *second* label. Compare
    // in index space, or the check silently passes the very case it exists to
    // catch — raw 0 against a present index of 1 looks like a difference and
    // renders «Вкл» commanding «Вкл».
    if (commanded != 0 && commanded != 1) {
      ADD_FAILURE() << "Control confirmation: a discrete item's command_value "
                       "is a raw state, so it must be 0 or 1, not "
                    << commanded;
      return {};
    }
    const int commanded_state = commanded != 0 ? 0 : 1;
    if (commanded_state == model->GetCurrentDiscreteState()) {
      ADD_FAILURE() << "Control confirmation: commanded state equals the "
                       "present one, so the review would show a no-op";
      return {};
    }
  }

  const std::u16string message =
      model->GetConfirmationMessage(commanded, second_stage);
  auto dialog_lifetime = StartDialogAwaitable(
      env.executor,
      dialog_service.RunMessageBox(message, model->GetSourceTitle(),
                                   MessageBoxMode::QuestionYesNoDefaultNo));
  PumpEventsFor(std::chrono::milliseconds{200});
  return dialog_lifetime;
}

// Fails when the dialog waiting to be grabbed shows an empty list.
//
// An empty list is the failure mode this generator cannot see by itself: the
// capture still writes a well-formed PNG of exactly the spec's dimensions, so
// `check_screenshots.py` passes and the blank image ships — which is how
// limits.png shipped with every field empty. The service-object dialog carries
// the same risk in a sharper form. Its component list is the selected device's
// HasComponent data variables, and the fixture has to state those references
// explicitly (`references` in screenshot_data.json); a device wired only by
// Organizes, which is the fixture's default, yields a populated device combo
// above a completely empty list.
bool ReportIfDialogListEmpty(const DialogSpec& spec) {
  QDialog* dialog = nullptr;
  for (QWidget* w : QApplication::topLevelWidgets()) {
    if (!w->isVisible())
      continue;
    if (auto* d = qobject_cast<QDialog*>(w)) {
      dialog = d;
      break;
    }
  }
  if (!dialog) {
    ADD_FAILURE() << "No visible dialog to check the list of, kind: "
                  << spec.kind;
    return false;
  }

  auto* list =
      dialog->findChild<QListWidget*>(QString{}, Qt::FindChildrenRecursively);
  if (!list) {
    ADD_FAILURE() << "No QListWidget in " << dialog->metaObject()->className()
                  << " for kind: " << spec.kind;
    return false;
  }
  if (list->count() == 0) {
    ADD_FAILURE() << "Empty list in " << dialog->metaObject()->className()
                  << " for kind: " << spec.kind
                  << " - the capture would ship a blank dialog";
    return false;
  }
  return true;
}

// Registers a representative spread of operator/engineering commands so the
// palette capture shows a realistic list. Titles go through Translate() (no
// Cyrillic literals in source); the generator loads no .ts, so they render in
// English.
void RegisterSampleCommands(CommandManager& manager) {
  const char* const titles[] = {
      "Acknowledge Alarm", "Acknowledge All Alarms",
      "Open Display",      "Export to CSV",
      "Write Value",       "Show Event Journal",
      "Add to Favorites",  "Print Preview",
      "Device Metrics",    "Refresh",
  };
  unsigned id = 1;
  for (const char* title : titles) {
    manager.RegisterCommand(
        CommandDescriptor{.command_id = id++, .title = Translate(title)});
  }
}

}  // namespace

bool CaptureDialog(const DialogSpec& spec, DialogEnvironment& env) {
  // Everything below grabs a QDialog out of `QApplication::topLevelWidgets()`
  // and closes it with `reject()`, so a dialog that the platform theme takes
  // over is both uncapturable and undismissable. The fixture pins
  // AA_DontUseNativeDialogs for exactly that reason (see SetUpTestSuite in
  // main.cpp); say so here rather than leaving the next person with a hung or
  // segfaulting run to diagnose.
  if (!QCoreApplication::testAttribute(Qt::AA_DontUseNativeDialogs)) {
    ADD_FAILURE() << "AA_DontUseNativeDialogs is not set: QMessageBox and "
                     "QFileDialog would become native modals the capture can "
                     "neither grab nor close (kind: "
                  << spec.kind << ")";
    return false;
  }

  // Per-call stubs — cheap to construct, no shared state.
  NullTransportFactory transport_factory;
  NullTaskManager task_manager;
  DialogServiceImplQt dialog_service;  // parent_widget = nullptr
  auto logger = std::make_shared<BoostLogger>(LOG_NAME("Screenshot"));

  // A spec may name its own target node; otherwise every node-driven kind
  // shares the fixture-wide one.
  const scada::NodeId dialog_node_id =
      spec.node_id.is_null() ? env.dialog_analog_node_id : spec.node_id;

  if (spec.kind == "login") {
    auto dialog_lifetime = BuildLoginDialog(env, transport_factory, logger);
    bool captured = GrabAndCloseVisibleDialogOrReport(spec);
    // Wait for the dialog coroutine to finish (reject() resolves it and the
    // dialog deleteLater's itself) before the per-call stubs above go out of
    // scope.
    WaitForDialogCompletion(dialog_lifetime);
    return captured;
  } else if (spec.kind == "limits") {
    auto dialog_lifetime =
        BuildLimitsDialog(env, dialog_node_id, task_manager, dialog_service);
    if (!dialog_lifetime)
      return false;
    bool captured = GrabAndCloseVisibleDialogOrReport(spec);
    WaitForDialogCompletion(dialog_lifetime);
    return captured;
  } else if (spec.kind == "write-manual") {
    auto dialog_lifetime =
        BuildWriteDialog(env, dialog_node_id, dialog_service, /*manual=*/true);
    if (!dialog_lifetime)
      return false;
    bool captured = GrabAndCloseVisibleDialogOrReport(spec);
    WaitForDialogCompletion(dialog_lifetime);
    return captured;
  } else if (spec.kind == "write-remote") {
    auto dialog_lifetime =
        BuildWriteDialog(env, dialog_node_id, dialog_service, /*manual=*/false);
    if (!dialog_lifetime)
      return false;
    bool captured = GrabAndCloseVisibleDialogOrReport(spec);
    WaitForDialogCompletion(dialog_lifetime);
    return captured;
  } else if (spec.kind == "control-confirm") {
    auto dialog_lifetime =
        BuildControlConfirmation(env, dialog_node_id, dialog_service,
                                 spec.second_stage, spec.command_value);
    if (!dialog_lifetime)
      return false;
    bool captured = GrabAndCloseVisibleDialogOrReport(spec);
    WaitForDialogCompletion(dialog_lifetime);
    return captured;
  } else if (spec.kind == "command-palette") {
    // The palette is a plain modal QDialog (not a CoSpawn'd coroutine like the
    // others): build it over a fixture command list and let the generic grab
    // pick it up. The manager can die once the ctor has copied its entries.
    CommandManager command_manager;
    RegisterSampleCommands(command_manager);
    auto* palette =
        new CommandPalette(nullptr, command_manager,
                           [](unsigned) -> CommandHandler* { return nullptr; });
    palette->setAttribute(Qt::WA_DeleteOnClose);
    palette->show();
    QApplication::processEvents();
    return GrabAndCloseVisibleDialogOrReport(spec);
  } else if (spec.kind == "about") {
    // Eagerly-shown self-owned modal; the generic grab rejects it and the
    // dialog deleteLater's itself.
    ShowAboutDialog(dialog_service);
    QApplication::processEvents();
    return GrabAndCloseVisibleDialogOrReport(spec);
  } else if (spec.kind == "change-password") {
    // Set Password for the fixture administrator (USER.5, the same user the
    // users-rbac capture shows). LocalEvents only collects the (never-fired)
    // completion toast.
    if (!env.node_service || !env.profile) {
      ADD_FAILURE() << "ChangePasswordDialog needs node_service + profile";
      return false;
    }
    const scada::NodeId user_id = NodeIdFromScadaString("USER.5");
    if (!FetchDialogNodeResident(*env.node_service, user_id)) {
      ADD_FAILURE() << "ChangePasswordDialog: fixture user not found";
      return false;
    }
    LocalEvents local_events;
    ShowChangePasswordDialog(
        dialog_service,
        ChangePasswordContext{.user_ = env.node_service->GetNode(user_id),
                              .executor_ = env.executor,
                              .local_events_ = local_events,
                              .profile_ = *env.profile});
    QApplication::processEvents();
    return GrabAndCloseVisibleDialogOrReport(spec);
  } else if (spec.kind == "time-range") {
    // The journal/graph period picker over its default (interval) range; the
    // date edits render the frozen fixture clock, so output is deterministic.
    if (!env.profile) {
      ADD_FAILURE() << "TimeRangeDialog needs a profile";
      return false;
    }
    auto dialog_lifetime = StartDialogAwaitable(
        env.executor,
        ShowTimeRangeDialog(
            dialog_service,
            TimeRangeContext{.profile_ = *env.profile,
                             .time_range_ = scada::RelativeTimeRange{},
                             .time_required_ = false}));
    bool captured = GrabAndCloseVisibleDialogOrReport(spec);
    WaitForDialogCompletion(dialog_lifetime);
    return captured;
  } else if (spec.kind == "message-box") {
    // The shared confirmation surface (DialogService::RunMessageBox), in its
    // question variant — the shape the two-stage confirms and apply/discard
    // prompts use. Mirrors the Excel-import "Apply changes?" call.
    auto dialog_lifetime = StartDialogAwaitable(
        env.executor, dialog_service.RunMessageBox(
                          Translate("Apply changes?"), Translate("Import"),
                          MessageBoxMode::QuestionYesNo));
    bool captured = GrabAndCloseVisibleDialogOrReport(spec);
    WaitForDialogCompletion(dialog_lifetime);
    return captured;
  } else if (spec.kind == "multi-create") {
    // Bulk TS/TI creation under the DataItems root. The device combo fills
    // from the fixture's Devices folder; the insert path is never taken.
    if (!env.node_service) {
      ADD_FAILURE() << "MultiCreateDialog needs a node_service";
      return false;
    }
    if (!FetchDialogNodeResident(*env.node_service,
                                 scada::devices::id::Devices)) {
      ADD_FAILURE() << "MultiCreateDialog: Devices folder not found";
      return false;
    }
    ShowMultiCreateDialog(dialog_service,
                          MultiCreateContext{*env.node_service, task_manager,
                                             scada::data_items::id::DataItems});
    QApplication::processEvents();
    return GrabAndCloseVisibleDialogOrReport(spec);
  } else if (spec.kind == "create-service-item") {
    // Service-object creation under the DataItems root - the modal the object
    // tree opens as «Создание сервисных объектов». The device combo fills from
    // the fixture's Devices folder and the component list from the selected
    // device's data variables; the insert path (PostInsertTask) is never taken.
    if (!env.node_service) {
      ADD_FAILURE() << "CreateServiceItemDialog needs a node_service";
      return false;
    }
    if (!FetchDialogNodeResident(*env.node_service,
                                 scada::devices::id::Devices)) {
      ADD_FAILURE() << "CreateServiceItemDialog: Devices folder not found";
      return false;
    }
    // A second wave for the devices themselves. FetchNodesResident makes the
    // node it is given and that node's children resident, so fetching the
    // Devices folder yields the device instances but not their components -
    // and CreateServiceItemModel reads its component list off the device, one
    // level further down, in its constructor. Without this the dialog renders
    // its combo over an empty list.
    std::vector<scada::NodeId> device_ids;
    for (const auto& [name, device] :
         GetNamedNodes(env.node_service->GetNode(scada::devices::id::Devices),
                       scada::devices::id::DeviceType)) {
      device_ids.push_back(device.node_id());
    }
    if (device_ids.empty()) {
      ADD_FAILURE() << "CreateServiceItemDialog: no devices under the Devices "
                       "folder";
      return false;
    }
    if (!scada::screenshot_generator::FetchNodesResident(*env.node_service,
                                                         device_ids)) {
      ADD_FAILURE() << "CreateServiceItemDialog: failed to fetch devices";
      return false;
    }

    ShowCreateServiceItemDialog(
        dialog_service, CreateServiceItemContext{
                            .node_service_ = *env.node_service,
                            .task_manager_ = task_manager,
                            .parent_id_ = scada::data_items::id::DataItems});
    QApplication::processEvents();
    if (!ReportIfDialogListEmpty(spec))
      return false;
    return GrabAndCloseVisibleDialogOrReport(spec);
  } else {
    ADD_FAILURE() << "Unknown dialog kind: " << spec.kind;
    return false;
  }
}
