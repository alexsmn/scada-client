#pragma once

#include "aui/dialog_service.h"
#include "base/awaitable.h"
#include "components/write/write_dialog.h"
#include "timed_data/timed_data_spec.h"

class WriteModel : private WriteContext,
                   public std::enable_shared_from_this<WriteModel> {
 public:
  explicit WriteModel(WriteContext&& context);

  void set_dialog_service(DialogService* dialog_service) {
    dialog_service_ = dialog_service;
  }

  bool discrete() const { return discrete_; }
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

  std::function<void()> current_change_handler;
  std::function<void()> condition_change_handler;
  std::function<void()> status_change_handler;
  std::function<void(bool ok)> completion_handler;

 private:
  void OnWriteComplete(const scada::Status& status);

  void StartWriting(bool second_stage);
  void StartWritingHelper();

  static Awaitable<void> CompleteWriteAsync(std::shared_ptr<Executor> executor,
                                            std::weak_ptr<WriteModel> model,
                                            promise<void> operation);
  static Awaitable<void> ConfirmAndStartWritingAsync(
      std::shared_ptr<Executor> executor,
      std::weak_ptr<WriteModel> model,
      promise<MessageBoxResult> prompt);
  static Awaitable<void> ReportWriteErrorAsync(
      std::shared_ptr<Executor> executor,
      std::function<void(bool ok)> completion_handler,
      DialogService& dialog_service,
      std::u16string message,
      std::u16string title);

  std::u16string GetConfirmationMessage(bool second_stage) const;

  DialogService* dialog_service_ = nullptr;

  TimedDataSpec spec_;
  bool discrete_ = false;
  bool locked_ = false;
  bool writing_ = false;
  bool write_selecting_ = false;
  double write_value_ = 0;

  bool has_condition_ = false;
  bool two_staged_ = false;
  TimedDataSpec condition_;
};
