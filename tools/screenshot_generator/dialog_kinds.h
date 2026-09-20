#pragma once

#include "dialog_capture.h"
#include "dialog_grab.h"
#include "dialog_pump.h"
#include "publish_guard.h"
#include "screenshot_wait.h"

#include "base/boost_log.h"
#include "scada/node_id.h"

#include <QApplication>
#include <transport/transport_factory.h>

#include <memory>
#include <span>

class DialogServiceImplQt;
class NullTaskManager;
struct DialogSpec;

namespace scada::screenshot_generator {

// TransportFactory that never produces a transport. LoginDialog & friends hold
// a `TransportFactory&` inside their DataServicesContext but only touch it on a
// real login attempt — we reject before that.
class NullTransportFactory : public transport::TransportFactory {
 public:
  transport::expected<transport::any_transport> CreateTransport(
      const transport::TransportString&,
      const transport::executor&,
      const transport::log_source&) override {
    return transport::ERR_NOT_IMPLEMENTED;
  }
};

// Everything one dialog capture can ask for.
//
// The dispatch table below holds one signature instead of sixteen, so this
// bundle carries the union of what the per-kind captures take; most use two or
// three of its members. Same arrangement, and for the same reason, as
// `StandaloneCaptureContext` on the screenshots side.
struct DialogCaptureContext {
  const DialogSpec& spec;
  DialogEnvironment& env;
  // Per-call stubs, built fresh for each capture by CaptureDialog and alive for
  // exactly as long as the capture is. Nothing here is shared between kinds.
  DialogServiceImplQt& dialog_service;
  NullTaskManager& task_manager;
  NullTransportFactory& transport_factory;
  std::shared_ptr<BoostLogger> logger;
  const CapturePublishGuard& publish_guard;
  // The spec's own `node` when it names one, else the fixture-wide analog item.
  // Which of the two a kind gets is how two captures of one kind can show two
  // states of the same dialog.
  scada::NodeId node_id;
};

// Makes the dialog's target node fully resident — attributes, property
// children, and the type-definition chain — so synchronous property reads
// (engineering units, limit bands, control flags) resolve when the dialog model
// is constructed.
inline bool FetchDialogNodeResident(AnyExecutor executor,
                                    NodeService& node_service,
                                    const scada::NodeId& node_id) {
  return FetchNodesResident(std::move(executor), node_service,
                            std::span<const scada::NodeId>{&node_id, 1});
}

// The tail every coroutine-driven dialog capture shares: grab and reject the
// dialog, then wait for the awaitable to unwind before the per-call stubs it
// borrowed leave scope.
//
// A null `lifetime` means the builder already reported why it could not build
// the dialog, so this reports nothing further.
template <class T>
bool GrabThenAwait(const DialogCaptureContext& context,
                   const std::shared_ptr<AwaitableResult<T>>& lifetime) {
  if (!lifetime)
    return false;
  const bool captured =
      GrabAndCloseVisibleDialogOrReport(context.spec, context.publish_guard);
  WaitForDialogCompletion(lifetime);
  return captured;
}

// The shorter tail, for a dialog whose factory shows it synchronously and
// leaves no awaitable to unwind.
inline bool GrabShownDialog(const DialogCaptureContext& context) {
  QApplication::processEvents();
  return GrabAndCloseVisibleDialogOrReport(context.spec,
                                           context.publish_guard);
}

// --- The kinds. One per `DialogSpec::kind`, each named by the fixture.
//
// Adding a dialog is two edits: a Capture* function in the matching
// dialog_*_captures.cpp, and a row in kDialogCaptures (dialog_capture.cpp).

// dialog_shell_captures.cpp — dialogs that need no address-space node.
bool CaptureLoginDialog(const DialogCaptureContext& context);
bool CaptureCommandPaletteDialog(const DialogCaptureContext& context);
bool CaptureAboutDialog(const DialogCaptureContext& context);
bool CaptureTimeRangeDialog(const DialogCaptureContext& context);
bool CaptureMessageBoxDialog(const DialogCaptureContext& context);
bool CaptureCsvExportDialog(const DialogCaptureContext& context);
bool CaptureAddFavouritesDialog(const DialogCaptureContext& context);
bool CaptureTransportDialog(const DialogCaptureContext& context);

// dialog_write_captures.cpp — the value-writing family, each driven by one
// fixture item whose properties have to be resident before the model is built.
bool CaptureLimitsDialog(const DialogCaptureContext& context);
bool CaptureManualWriteDialog(const DialogCaptureContext& context);
bool CaptureRemoteWriteDialog(const DialogCaptureContext& context);
bool CaptureControlConfirmationDialog(const DialogCaptureContext& context);

// dialog_node_captures.cpp — dialogs that browse the address space, so they
// need whole subtrees resident rather than one item.
bool CaptureChangePasswordDialog(const DialogCaptureContext& context);
bool CaptureMultiCreateDialog(const DialogCaptureContext& context);
bool CaptureCreateServiceItemDialog(const DialogCaptureContext& context);

}  // namespace scada::screenshot_generator
