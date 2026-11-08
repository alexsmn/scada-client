#pragma once

#include "base/any_executor.h"

#include "aui/dialog_service.h"
#include "base/awaitable.h"
#include "modules/write/write_dialog.h"
#include "scada/co_result.h"
#include "scada/status.h"
#include "timed_data/timed_data_spec.h"

#include <optional>
#include <string_view>

// Parses a number the operator typed into the Control dialog. Accepts either
// a decimal point or a decimal comma, so «12,5» typed on a Russian keyboard is
// the same value as «12.5» (backlog 912); surrounding spaces are ignored.
// Returns nullopt for anything else, including a trailing unit or a second
// separator.
std::optional<double> ParseOperatorNumber(std::u16string_view text);

class WriteModel : private WriteContext,
                   public std::enable_shared_from_this<WriteModel> {
 public:
  explicit WriteModel(WriteContext&& context);
  // Releases a selection still outstanding: the dialog that owns this model
  // has gone, by whichever route -- the window button, Escape, a parent
  // destroying it while the confirmation is up (backlog 711).
  ~WriteModel();

  void set_dialog_service(DialogService* dialog_service) {
    dialog_service_ = dialog_service;
  }

  bool discrete() const { return discrete_; }
  // True when the operator is entering a value into the point by hand, false
  // when this is a command sent out to a device. The two read differently to
  // an operator and are labelled differently (docs/client/ux/dialogs.md §3).
  bool manual() const { return manual_; }
  bool lock_allowed() const { return manual_; }
  bool locked() const { return locked_; }
  bool has_condition() const { return has_condition_; }
  bool two_staged() const { return two_staged_; }

  std::u16string GetWindowTitle() const;
  std::u16string GetSourceTitle() const;
  std::u16string GetCurrentValue(bool formatted) const;
  std::u16string GetStatusText() const;
  bool IsConditionOk() const;

  std::vector<std::u16string> GetDiscreteStates() const;
  int GetCurrentDiscreteState() const;

  std::u16string GetAnalogUnits() const;

  void Write(double value, bool lock);

  // Tells the operator why the value they typed was refused, and leaves the
  // dialog open so they can correct it. Nothing is written and
  // `completion_handler` is not called: this reports input the dialog rejected
  // before any value reached the point, which is not a completed write.
  //
  // Lives on the model rather than in the dialog because showing a message box
  // means awaiting a lazy awaitable, and the model is what holds the executor
  // to spawn it on.
  void ReportInputError(std::u16string message);

  // The review an operator answers before `value` is sent: the point's present
  // reading, the commanded value, and an irreversibility warning.
  // `second_stage` prefixes the operate half of a select-before-operate
  // command, where the outstation is already selected and waiting.
  //
  // Public so the documentation screenshot generator captures the real prompt
  // rather than a copy that can drift from it.
  std::u16string GetConfirmationMessage(double value, bool second_stage) const;

  std::function<void()> current_change_handler;
  std::function<void()> condition_change_handler;
  std::function<void()> status_change_handler;
  std::function<void(bool ok)> completion_handler;

 private:
  void OnWriteComplete(const scada::Status& status);
  // The error an operator sees when the select stage of a two-stage control is
  // refused; Bad_NotSupported says the object's device cannot select.
  static std::u16string GetSelectRefusalMessage(const scada::Status& status);
  // Reports a completed command as a local event, if the profile asks for it.
  void ReportSuccess();

  void StartWriting(bool second_stage);
  void StartWritingHelper();
  // Re-sends the select stage as a 2.5-style flagged value write, after the
  // Server refused the Control object's Select as unknown.
  void SelectByLegacyWrite();

  // The item's Control object (`<item>!Control`), which carries the
  // Select/Operate/Cancel methods.
  scada::node ControlNode() const;

  // Releases a selection the operator decided not to act on. A refused
  // release is reported as a local event, never as a dialog.
  void CancelOutstandingSelect();

  // Awaits the Cancel and, when the device refuses it, reports to
  // `local_events` that the selection of `source_title` may still be armed.
  // Bad_NotSupported — a driver that never selects — reports nothing. Takes no
  // model: the dialog may already be gone when the device answers.
  // `local_events` is checked against its `lifetime()` before the report, so
  // an answer arriving after the session was torn down reports nothing.
  static Awaitable<void> ReportCancelRefusalAsync(scada::CoStatus operation,
                                                  LocalEvents* local_events,
                                                  std::u16string source_title);

  // Awaits a select and hands its outcome to `model`. When the model is gone
  // by then -- the operator closed the dialog while the select was in flight
  // -- a select the device accepted is released with `cancel` instead, since
  // nobody is left to operate or decline it. `cancel` is built eagerly and is
  // lazy, so it is never sent while the model is alive to decide.
  static Awaitable<void> CompleteSelectAsync(std::weak_ptr<WriteModel> model,
                                             scada::CoStatus select,
                                             scada::CoStatus cancel,
                                             LocalEvents* local_events,
                                             std::u16string source_title);

  // The legacy twin of CompleteSelectAsync, for a Server before 2.6: awaits
  // the flagged-write select and hands its outcome to `model`. When the model
  // is gone by then, a select the device accepted cannot be released (such a
  // Server has no Cancel), so the held selection of `source_title` is reported
  // to `local_events` instead -- checked against its `lifetime()` first.
  // Parameters are by value: the frame outlives the CoSpawn call.
  static Awaitable<void> CompleteLegacySelectAsync(
      std::weak_ptr<WriteModel> model,
      scada::CoStatus select,
      LocalEvents* local_events,
      std::u16string source_title);

  // Reports that a Server before 2.6 left `source_title` selected, since it
  // has no Cancel. Synchronous; a null `local_events` reports nothing.
  static void ReportLegacySelectionHeld(LocalEvents* local_events,
                                        std::u16string_view source_title);

  static Awaitable<void> CompleteWriteAsync(AnyExecutor executor,
                                            std::weak_ptr<WriteModel> model,
                                            scada::CoStatus operation);
  // Owns `message`/`title` for the lifetime of the confirmation prompt: the
  // RunMessageBox awaitable is created and awaited inside this coroutine, so
  // the string_views it takes stay valid (a prompt created by the caller would
  // bind views into caller locals that die before the lazy coroutine reads
  // them).
  static Awaitable<void> ConfirmAndStartWritingAsync(
      AnyExecutor executor,
      std::weak_ptr<WriteModel> model,
      DialogService& dialog_service,
      std::u16string message,
      std::u16string title);
  static Awaitable<void> ReportWriteErrorAsync(
      AnyExecutor executor,
      std::function<void(bool ok)> completion_handler,
      DialogService& dialog_service,
      std::u16string message,
      std::u16string title);
  // Owns `message`/`title` for the lifetime of the prompt, for the same reason
  // ConfirmAndStartWritingAsync does. Unlike ReportWriteErrorAsync this
  // completes nothing when the box is dismissed — there is no write in flight.
  static Awaitable<void> ShowInputErrorAsync(DialogService& dialog_service,
                                             std::u16string message,
                                             std::u16string title);

  DialogService* dialog_service_ = nullptr;

  TimedDataSpec spec_;
  bool discrete_ = false;
  bool locked_ = false;
  bool writing_ = false;
  bool write_selecting_ = false;
  // A select has completed and has been neither operated nor cancelled.
  // Distinct from write_selecting_, which StartWritingHelper clears before the
  // confirmation prompt is even shown.
  bool select_outstanding_ = false;
  double write_value_ = 0;

  bool has_condition_ = false;
  bool two_staged_ = false;
  // The Server has no per-item Control object -- a Server from before 2.6,
  // which refuses the Select call as an unknown node (IsMissingOnServer). The
  // two stages then go the way the 2.5 Client sent them: a value write flagged
  // as a select, then a plain value write, and no Cancel, which such a Server
  // does not have.
  bool legacy_control_ = false;
  TimedDataSpec condition_;
};
