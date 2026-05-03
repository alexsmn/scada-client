#include "dialog_capture.h"

#include "screenshot_config.h"
#include "screenshot_output.h"
#include "screenshot_wait.h"

#include "aui/qt/dialog_service_impl_qt.h"
#include "base/console_logger.h"
#include "base/any_executor.h"
#include "components/limits/limit_dialog.h"
#include "components/login/login_dialog.h"
#include "components/write/write_dialog.h"
#include "node_service/node_ref.h"
#include "node_service/node_fetch_status.h"
#include "node_service/node_service.h"
#include "profile/profile.h"
#include "scada/data_services_factory.h"
#include "scada/logging.h"
#include "scada/node_id.h"
#include "services/task_manager.h"
#include "timed_data/timed_data_service.h"

#include <QApplication>
#include <QDialog>
#include <QPixmap>
#include <QString>
#include <QWidget>
#include <transport/transport_factory.h>

#include <gtest/gtest.h>
#include <optional>
#include <stdexcept>
#include <type_traits>

namespace {

using screenshot_generator::WaitForPendingNodeLoads;

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
  CoSpawn(std::move(executor),
          [result, awaitable = std::move(awaitable)]() mutable
              -> Awaitable<void> {
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

Awaitable<void> RejectTaskManagerCallAsync() {
  throw std::runtime_error{"Screenshot capture task manager is not available"};
  co_return;
}

template <typename T>
Awaitable<T> RejectTaskManagerCallAsync() {
  throw std::runtime_error{"Screenshot capture task manager is not available"};
  co_return T{};
}

// Dummy TaskManager. We never press OK on captured dialogs, so no
// task is ever posted. Each method still rejects through a coroutine in case
// something slips through.
class NullTaskManager : public TaskManager {
 public:
  Awaitable<void> PostTask(std::u16string_view,
                           const TaskLauncher&) override {
    return RejectTaskManagerCallAsync();
  }
  Awaitable<scada::NodeId> PostInsertTask(
      const scada::NodeState&) override {
    return RejectTaskManagerCallAsync<scada::NodeId>();
  }
  Awaitable<void> PostUpdateTask(const scada::NodeId&,
                                 scada::NodeAttributes,
                                 scada::NodeProperties) override {
    return RejectTaskManagerCallAsync();
  }
  Awaitable<void> PostDeleteTask(const scada::NodeId&) override {
    return RejectTaskManagerCallAsync();
  }
  Awaitable<void> PostAddReference(const scada::NodeId&,
                                   const scada::NodeId&,
                                   const scada::NodeId&) override {
    return RejectTaskManagerCallAsync();
  }
  Awaitable<void> PostDeleteReference(const scada::NodeId&,
                                      const scada::NodeId&,
                                      const scada::NodeId&) override {
    return RejectTaskManagerCallAsync();
  }
};

bool FetchAndWaitForPendingNodeLoads(NodeService& node_service,
                                     const NodeRef& node,
                                     const NodeFetchStatus& requested_status) {
  if (!node) {
    return false;
  }

  node.Fetch(requested_status);
  return WaitForPendingNodeLoads(node_service);
}

// Scans top-level widgets for a visible QDialog, resizes it to the
// spec dims, grabs a pixmap, and rejects the dialog so whoever called
// `show()` can finish the cleanup path (deleteLater in most factories).
bool GrabAndCloseVisibleDialog(const DialogSpec& spec) {
  QDialog* dialog = nullptr;
  for (int attempt = 0; attempt < 50 && !dialog; ++attempt) {
    QApplication::processEvents();
    for (QWidget* w : QApplication::topLevelWidgets()) {
      if (w->isVisible()) {
        if (auto* d = qobject_cast<QDialog*>(w)) {
          dialog = d;
          break;
        }
      }
    }
  }
  if (!dialog)
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
  ADD_FAILURE() << "No visible dialog for kind: " << spec.kind;
  return false;
}

void WaitForDialogCompletion(
    const std::shared_ptr<DialogAwaitableResult<void>>& result) {
  for (int i = 0; i < 200 && !IsDialogAwaitableReady(result); ++i)
    QApplication::processEvents(QEventLoop::AllEvents |
                                    QEventLoop::WaitForMoreEvents,
                                20);

  if (!IsDialogAwaitableReady(result))
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
BuildLoginDialog(
    DialogEnvironment& env,
    NullTransportFactory& transport_factory,
    const std::shared_ptr<Logger>& logger) {
  DataServicesContext services_context{logger, env.executor,
                                       transport_factory,
                                       scada::ServiceLogParams{}};
  auto dialog_lifetime = StartDialogAwaitable(
      env.executor,
      ExecuteLoginDialog(env.executor, std::move(services_context)));
  QApplication::processEvents();
  return dialog_lifetime;
}

// Limits dialog: needs a NodeRef to an analog variable plus a
// TaskManager for the (never-taken) write path. We pull the configured
// dialog analog node out of the fixture; in the current fixture that is
// "Температура нагрева", the analog node the docs images target. Limit
// values still render empty until the fixture grows
// HasProperty support for AnalogItemType_Limit{Hi,Lo,HiHi,LoLo} (gap #2).
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
  if (!FetchAndWaitForPendingNodeLoads(
          *env.node_service, node, NodeFetchStatus::NodeOnly())) {
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
    ADD_FAILURE()
        << "WriteDialog needs timed_data_service + profile + node_service in env";
    return {};
  }
  auto node = env.node_service->GetNode(env.dialog_analog_node_id);
  if (!node) {
    ADD_FAILURE() << "WriteDialog: configured fixture node not found";
    return {};
  }
  if (!FetchAndWaitForPendingNodeLoads(
          *env.node_service, node, NodeFetchStatus::NodeOnly())) {
    ADD_FAILURE() << "WriteDialog: failed to fetch configured fixture node";
    return {};
  }
  auto dialog_lifetime = StartDialogAwaitable(env.executor, ExecuteWriteDialog(
      dialog_service, WriteContext{.executor_ = env.executor,
                                   .timed_data_service_ =
                                       *env.timed_data_service,
                                   .node_id_ = env.dialog_analog_node_id,
                                   .profile_ = *env.profile,
                                   .manual_ = manual}));
  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();
  return dialog_lifetime;
}

}  // namespace

bool CaptureDialog(const DialogSpec& spec, DialogEnvironment& env) {
  // Per-call stubs — cheap to construct, no shared state.
  NullTransportFactory transport_factory;
  NullTaskManager task_manager;
  DialogServiceImplQt dialog_service;  // parent_widget = nullptr
  auto logger = std::make_shared<ConsoleLogger>();

  if (spec.kind == "login") {
    [[maybe_unused]] auto dialog_lifetime =
        BuildLoginDialog(env, transport_factory, logger);
    return GrabAndCloseVisibleDialogOrReport(spec);
  } else if (spec.kind == "limits") {
    auto dialog_lifetime =
        BuildLimitsDialog(env, task_manager, dialog_service);
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
  } else {
    ADD_FAILURE() << "Unknown dialog kind: " << spec.kind;
    return false;
  }
}
