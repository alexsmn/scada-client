#pragma once

#include "base/memory/weak_ptr.h"
#include "components/write/write_dialog.h"
#include "timed_data/timed_data_spec.h"

class WriteModel : private WriteContext {
 public:
  explicit WriteModel(WriteContext&& context);

  void set_dialog_service(DialogService* dialog_service) {
    dialog_service_ = dialog_service;
  }

  bool discrete() const { return discrete_; }
  bool lock_allowed() const { return manual_; }
  bool has_condition() const { return has_condition_; }

  base::string16 GetWindowTitle() const;
  base::string16 GetSourceTitle() const;
  base::string16 GetCurrentValue(bool formatted) const;
  base::string16 GetStatusText() const;
  bool IsConditionOk() const;

  std::vector<base::string16> GetDiscreteStates() const;
  int GetCurrentDiscreteState() const;

  base::string16 GetAnalogUnits() const;

  void Write(double value, bool lock);

  std::function<void()> current_change_handler;
  std::function<void()> condition_change_handler;
  std::function<void()> status_change_handler;
  std::function<void(bool ok)> completion_handler;

 private:
  void OnWriteComplete(const scada::Status& status);

  DialogService* dialog_service_ = nullptr;

  rt::TimedDataSpec spec_;
  bool discrete_ = false;
  bool write_selecting_ = false;
  double write_value_ = 0;

  bool has_condition_ = false;
  rt::TimedDataSpec condition_;

  base::WeakPtrFactory<WriteModel> weak_factory_{this};
};
