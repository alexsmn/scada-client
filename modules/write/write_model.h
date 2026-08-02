#pragma once

#include "base/any_executor.h"

#include "aui/dialog_service.h"
#include "base/awaitable.h"
#include "modules/write/write_dialog.h"
#include "scada/co_result.h"
#include "scada/status.h"
#include "timed_data/timed_data_spec.h"

class WriteModel : private WriteContext,
                   public std::enable_shared_from_this<WriteModel> {
 public:
  explicit WriteModel(WriteContext&& context);

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

  void StartWriting(bool second_stage);
  void StartWritingHelper();

  // The item's Control object (`<item>!Control`), which carries the
  // Select/Operate/Cancel methods.
  scada::node ControlNode() const;

  // Releases a selection the operator decided not to act on.
  void CancelOutstandingSelect();

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
  TimedDataSpec condition_;
};
