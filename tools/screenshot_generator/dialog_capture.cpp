#include "dialog_capture.h"

#include "screenshot_config.h"
#include "screenshot_output.h"
#include "screenshot_wait.h"

#include "aui/qt/dialog_service_impl_qt.h"
#include "aui/translation.h"
#include "base/any_executor.h"
#include "base/boost_log.h"
#include "base/memory_settings_store.h"
#include "controller/command_manager.h"
#include "main_window/command_palette_qt.h"
#include "modules/limits/limit_dialog.h"
#include "modules/login/login_dialog.h"
#include "modules/write/write_dialog.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "profile/profile.h"
#include "scada/data_services_factory.h"
#include "scada/logging.h"
#include "scada/node_id.h"
#include "services/task_manager.h"
#include "timed_data/timed_data_service.h"

#include <QApplication>
#include <QDialog>
#include <QElapsedTimer>
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

Awaitable<scada::Status> RejectTaskManagerCallAsync() {
  co_return scada::StatusCode::Bad;
}

Awaitable<scada::StatusOr<scada::NodeId>> RejectPostInsertTaskAsync() {
  co_return scada::StatusCode::Bad;
}

// Dummy TaskManager. We never press OK on captured dialogs, so no
// task is ever posted. Each method still rejects through a coroutine in case
// something slips through.
class NullTaskManager : public TaskManager {
 public:
  Awaitable<scada::Status> PostTask(std::u16string_view,
                                    const TaskLauncher&) override {
    return RejectTaskManagerCallAsync();
  }
  Awaitable<scada::StatusOr<scada::NodeId>> PostInsertTask(
      const scada::NodeState&) override {
    return RejectPostInsertTaskAsync();
  }
  Awaitable<scada::Status> PostUpdateTask(const scada::NodeId&,
                                          scada::NodeAttributes,
                                          scada::NodeProperties) override {
    return RejectTaskManagerCallAsync();
  }
  Awaitable<scada::Status> PostDeleteTask(const scada::NodeId&) override {
    return RejectTaskManagerCallAsync();
  }
  Awaitable<scada::Status> PostAddReference(const scada::NodeId&,
                                            const scada::NodeId&,
                                            const scada::NodeId&) override {
    return RejectTaskManagerCallAsync();
  }
  Awaitable<scada::Status> PostDeleteReference(const scada::NodeId&,
                                               const scada::NodeId&,
                                               const scada::NodeId&) override {
    return RejectTaskManagerCallAsync();
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

  QPixmap pixmap = dialog->grab();
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
  auto settings_store = std::make_shared<MemorySettingsStore>();
  settings_store->SetString16("User", u"root");
  settings_store->SetString16("UserList", u"root");
  settings_store->SetString("Host", "127.0.0.1");
  auto dialog_lifetime = StartDialogAwaitable(
      env.executor,
      ExecuteLoginDialog(env.executor, std::move(services_context),
                         std::move(settings_store)));
  QApplication::processEvents();
  return dialog_lifetime;
}

// Limits dialog: needs a NodeRef to an analog variable plus a
// TaskManager for the (never-taken) write path. We pull the configured
// dialog analog node out of the fixture; in the current fixture that is
// "Температура нагрева", the analog node the docs images target. The
// resident fetch below resolves property reads, so limit fields render
// whenever the fixture node carries limit_{lolo,lo,hi,hihi} properties
// (the current dialog node intentionally has none).
std::shared_ptr<DialogAwaitableResult<void>> BuildLimitsDialog(
    DialogEnvironment& env,
    NullTaskManager& task_manager,
    DialogServiceImplQt& dialog_service) {
  if (!env.node_service) {
    ADD_FAILURE() << "LimitsDialog needs a node_service in DialogEnvironment";
    return {};
  }
  auto node = env.node_service->GetNode(env.dialog_analog_node_id);
  if (!node) {
    ADD_FAILURE() << "LimitsDialog: configured fixture node not found";
    return {};
  }
  if (!FetchDialogNodeResident(*env.node_service, env.dialog_analog_node_id)) {
    ADD_FAILURE() << "LimitsDialog: failed to fetch configured fixture node";
    return {};
  }
  auto dialog_lifetime = StartDialogAwaitable(
      env.executor,
      ShowLimitsDialog(dialog_service, LimitDialogContext{node, task_manager}));
  QApplication::processEvents();
  return dialog_lifetime;
}

// Write dialog family. `manual` picks between "Manual Input" (TI
// manual override) and "Control" (remote device control). Target node
// is configured in `screenshot_data.json` — the analog TI node the docs
// images use. The model treats it as continuous (not discrete) because
// no HasTsFormat reference is wired up, which matches the ti-*-control
// docs images. The ts- variants will render identically until the
// fixture grows a proper TS (with logical / discrete semantics).
//
// After show() we pump events aggressively: the real TimedDataServiceImpl
// fulfils the current value through an async subscribe+read chain routed
// through TestExecutor and Qt's event loop, and the UI only updates via
// current_change_handler once the DataValue lands. Too few pumps leaves
// the "Current value:" label blank in the grab.
std::shared_ptr<DialogAwaitableResult<void>> BuildWriteDialog(
    DialogEnvironment& env,
    DialogServiceImplQt& dialog_service,
    bool manual) {
  if (!env.timed_data_service || !env.profile || !env.node_service) {
    ADD_FAILURE() << "WriteDialog needs timed_data_service + profile + "
                     "node_service in env";
    return {};
  }
  auto node = env.node_service->GetNode(env.dialog_analog_node_id);
  if (!node) {
    ADD_FAILURE() << "WriteDialog: configured fixture node not found";
    return {};
  }
  if (!FetchDialogNodeResident(*env.node_service, env.dialog_analog_node_id)) {
    ADD_FAILURE() << "WriteDialog: failed to fetch configured fixture node";
    return {};
  }
  auto dialog_lifetime = StartDialogAwaitable(
      env.executor,
      ExecuteWriteDialog(
          dialog_service,
          WriteContext{.executor_ = env.executor,
                       .timed_data_service_ = *env.timed_data_service,
                       .node_id_ = env.dialog_analog_node_id,
                       .profile_ = *env.profile,
                       .manual_ = manual}));
  PumpEventsFor(std::chrono::milliseconds{200});
  return dialog_lifetime;
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
  // Per-call stubs — cheap to construct, no shared state.
  NullTransportFactory transport_factory;
  NullTaskManager task_manager;
  DialogServiceImplQt dialog_service;  // parent_widget = nullptr
  auto logger = std::make_shared<BoostLogger>(LOG_NAME("Screenshot"));

  if (spec.kind == "login") {
    auto dialog_lifetime = BuildLoginDialog(env, transport_factory, logger);
    bool captured = GrabAndCloseVisibleDialogOrReport(spec);
    // Wait for the dialog coroutine to finish (reject() resolves it and the
    // dialog deleteLater's itself) before the per-call stubs above go out of
    // scope.
    WaitForDialogCompletion(dialog_lifetime);
    return captured;
  } else if (spec.kind == "limits") {
    auto dialog_lifetime = BuildLimitsDialog(env, task_manager, dialog_service);
    if (!dialog_lifetime)
      return false;
    bool captured = GrabAndCloseVisibleDialogOrReport(spec);
    WaitForDialogCompletion(dialog_lifetime);
    return captured;
  } else if (spec.kind == "write-manual") {
    auto dialog_lifetime =
        BuildWriteDialog(env, dialog_service, /*manual=*/true);
    if (!dialog_lifetime)
      return false;
    bool captured = GrabAndCloseVisibleDialogOrReport(spec);
    WaitForDialogCompletion(dialog_lifetime);
    return captured;
  } else if (spec.kind == "write-remote") {
    auto dialog_lifetime =
        BuildWriteDialog(env, dialog_service, /*manual=*/false);
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
  } else {
    ADD_FAILURE() << "Unknown dialog kind: " << spec.kind;
    return false;
  }
}
