// The value-writing dialog family: limits, the two WriteDialog variants, and
// the operator's control confirmation.
//
// What they share is one fixture item whose properties have to be resident, and
// whose current value has to have landed, before the dialog model is built —
// both of which are silent failures that still write a well-formed PNG.

#include "dialog_kinds.h"

#include "null_task_manager.h"
#include "screenshot_config.h"

#include "aui/qt/dialog_service_impl_qt.h"
#include "modules/limits/limit_dialog.h"
#include "modules/write/write_dialog.h"
#include "modules/write/write_model.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "profile/profile.h"
#include "timed_data/timed_data_service.h"
#include "timed_data/timed_data_spec.h"

#include <QApplication>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <optional>
#include <utility>

namespace scada::screenshot_generator {
namespace {

// Resolves the spec's target node and makes it resident, reporting through
// `what` on a miss. Null when the capture cannot proceed.
//
// A node without the properties its dialog reads captures an empty dialog,
// which is how limits.png shipped with every field blank — so a miss here has
// to fail rather than fall through.
NodeRef ResolveDialogNode(const DialogCaptureContext& context,
                          const char* what) {
  DialogEnvironment& env = context.env;
  if (!env.node_service) {
    ADD_FAILURE() << what << " needs a node_service in DialogEnvironment";
    return {};
  }
  auto node = env.node_service->GetNode(context.node_id);
  if (!node) {
    ADD_FAILURE() << what << ": configured fixture node not found";
    return {};
  }
  if (!FetchDialogNodeResident(env.executor, *env.node_service,
                               context.node_id)) {
    ADD_FAILURE() << what << ": failed to fetch configured fixture node";
    return {};
  }
  return node;
}

// Waits for the item's current value to land before the dialog or model exists.
//
// WriteDialog reads the discrete state once, in its constructor, and only ever
// refreshes the "Current value:" label afterwards — so a value that lands later
// leaves the combo showing GetCurrentDiscreteState()'s get_or(true) default
// beside a label that has since corrected itself, and the dialog proposes the
// state it is already in. Whether that happened used to depend on whether an
// earlier capture in the same run had already subscribed to the same item:
// ts-remote-control-enabled.png rendered correctly only because
// ts-manual-control.png names the same node and ran first.
void WarmUpCurrentValue(TimedDataService& timed_data_service,
                        const scada::NodeId& node_id) {
  TimedDataSpec warm_up{timed_data_service, node_id};
  PumpEventsUntil([&warm_up] { return !warm_up.current().value.is_null(); },
                  std::chrono::seconds{2});
}

// Reports unless the write family has everything it needs on `env`.
bool HasWriteServices(const DialogCaptureContext& context, const char* what) {
  if (context.env.timed_data_service && context.env.profile &&
      context.env.node_service) {
    return true;
  }
  ADD_FAILURE() << what
                << " needs timed_data_service + profile + node_service in env";
  return false;
}

// Both WriteDialog variants, which differ only in `manual`.
//
// Which of the two dialogs the model builds is the node's doing, not the kind's:
// WriteModel asks TimedDataSpec::logical(), so an AnalogItemType node renders
// the editable numeric field with its engineering unit (the ti-* captures) and
// a DiscreteItemType node renders the read-only two-state combo labelled from
// its HasTsFormat target (the ts-* captures, whose specs name TS.201/TS.202).
bool CaptureWriteDialog(const DialogCaptureContext& context, bool manual) {
  const char* const what = manual ? "Manual WriteDialog" : "Remote WriteDialog";
  if (!HasWriteServices(context, what))
    return false;
  if (!ResolveDialogNode(context, what))
    return false;

  DialogEnvironment& env = context.env;
  WarmUpCurrentValue(*env.timed_data_service, context.node_id);

  WriteContext write_context{.executor_ = env.executor,
                             .timed_data_service_ = *env.timed_data_service,
                             .node_id_ = context.node_id,
                             .profile_ = *env.profile,
                             .manual_ = manual};
  auto lifetime = StartAwaitable(
      env.executor,
      ExecuteWriteDialog(context.dialog_service, std::move(write_context)));
  // The real TimedDataServiceImpl fulfils the current value through an async
  // subscribe+read chain routed through the executor and Qt's event loop, and
  // the UI only updates via current_change_handler once the DataValue lands.
  PumpEventsFor(std::chrono::milliseconds{200});
  return GrabThenAwait(context, lifetime);
}

}  // namespace

bool CaptureLimitsDialog(const DialogCaptureContext& context) {
  // Needs a NodeRef to an analog variable plus a TaskManager for the
  // (never-taken) write path. In the current fixture the default node is
  // "Температура нагрева", the analog node the docs images target; the resident
  // fetch resolves property reads, so the four limit fields render from that
  // node's limit_{lolo,lo,hi,hihi} properties.
  auto node = ResolveDialogNode(context, "LimitsDialog");
  if (!node)
    return false;

  auto lifetime = StartAwaitable(
      context.env.executor,
      ShowLimitsDialog(context.dialog_service,
                       LimitDialogContext{context.env.executor, node,
                                          context.task_manager}));
  QApplication::processEvents();
  return GrabThenAwait(context, lifetime);
}

bool CaptureManualWriteDialog(const DialogCaptureContext& context) {
  return CaptureWriteDialog(context, /*manual=*/true);
}

bool CaptureRemoteWriteDialog(const DialogCaptureContext& context) {
  return CaptureWriteDialog(context, /*manual=*/false);
}

bool CaptureControlConfirmationDialog(const DialogCaptureContext& context) {
  // The operator's review of a control command. The prompt itself comes from a
  // real WriteModel over a fixture item, so the capture cannot drift from the
  // shipped wording; it is then shown through the ordinary DialogService
  // question box, which is what StartWriting() does.
  //
  // `spec.second_stage` picks which of the two prompts is rendered: the
  // ordinary one-shot review, or the operate stage of a select-before-operate
  // command, which GetConfirmationMessage prefixes with "the remote device is
  // ready". The select is not actually issued in either case — a Control-object
  // Call needs a server, and this capture is of the review step, not of the
  // two-phase exchange (that is covered by the iec104 tier E2E and by
  // WriteModelTest.TwoStagedControlSelectsConfirmsThenOperates).
  constexpr const char* kWhat = "Control confirmation";
  if (!HasWriteServices(context, kWhat))
    return false;

  DialogEnvironment& env = context.env;
  if (!FetchDialogNodeResident(env.executor, *env.node_service,
                               context.node_id)) {
    ADD_FAILURE() << kWhat << ": fixture node not found";
    return false;
  }
  // The review quotes the present reading and WriteModel resolves it once — and
  // on a two-state item the get_or(true) default *is* one of the two states, so
  // without this the review renders a plausible no-op instead of failing.
  WarmUpCurrentValue(*env.timed_data_service, context.node_id);

  auto model = std::make_shared<WriteModel>(
      WriteContext{.executor_ = env.executor,
                   .timed_data_service_ = *env.timed_data_service,
                   .node_id_ = context.node_id,
                   .profile_ = *env.profile,
                   .manual_ = false});
  model->set_dialog_service(&context.dialog_service);

  // Default: a value clear of the analog fixture reading, so Present and
  // Command differ. A discrete item overrides it — its states are not on the
  // same scale — as does any capture wanting a particular commanded value.
  constexpr double kDefaultCommandedValue = 12.5;
  const double commanded =
      context.spec.command_value.value_or(kDefaultCommandedValue);

  // The review exists so the operator can compare what the point reads now
  // against what the command makes it, so a capture whose two halves coincide
  // documents nothing. On a discrete item an out-of-range state is the sharper
  // version of the same fault: it has no label, so the prompt quotes empty text
  // and the capture still writes a well-formed PNG that
  // `check_screenshots.py` accepts — the `limits.png` failure mode again.
  if (model->discrete()) {
    // `commanded` is the item's raw value, which is not an index into
    // GetDiscreteStates(): WriteModel labels a state by inverting the raw value
    // (`get_or(true) ? 0 : 1`), so raw 0 is the *second* label. Compare in index
    // space, or the check silently passes the very case it exists to catch —
    // raw 0 against a present index of 1 looks like a difference and renders
    // «Вкл» commanding «Вкл».
    if (commanded != 0 && commanded != 1) {
      ADD_FAILURE() << kWhat
                    << ": a discrete item's command_value is a raw state, so "
                       "it must be 0 or 1, not "
                    << commanded;
      return false;
    }
    const int commanded_state = commanded != 0 ? 0 : 1;
    if (commanded_state == model->GetCurrentDiscreteState()) {
      ADD_FAILURE() << kWhat
                    << ": commanded state equals the present one, so the "
                       "review would show a no-op";
      return false;
    }
  }

  const std::u16string message =
      model->GetConfirmationMessage(commanded, context.spec.second_stage);
  auto lifetime = StartAwaitable(
      env.executor,
      context.dialog_service.RunMessageBox(
          message, model->GetSourceTitle(),
          MessageBoxMode::QuestionYesNoDefaultNo));
  PumpEventsFor(std::chrono::milliseconds{200});
  return GrabThenAwait(context, lifetime);
}

}  // namespace scada::screenshot_generator
