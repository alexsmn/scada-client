#pragma once

#include "components/vidicon_display/tester/qt/variable_timed_data_service.h"
#include "controller_delegate.h"

class ControllerDelegateImpl : public ControllerDelegate {
 public:
  virtual void SetTitle(std::u16string_view title) override {}
  virtual void ShowPopupMenu(unsigned resource_id,
                             const aui::Point& point,
                             bool right_click) override {}
  virtual void SetModified(bool modified) override {}
  virtual void Close() override {}
  virtual void OpenView(const WindowDefinition& def) override {}
  virtual void ExecuteDefaultNodeCommand(const NodeRef& node) override {}
  virtual ContentsModel* GetActiveContentsModel() override { return nullptr; }
  virtual void AddContentsObserver(ContentsObserver& observer) override {}
  virtual void RemoveContentsObserver(ContentsObserver& observer) override {}
  virtual void Focus() override {}
};

struct TesterState {
  ControllerDelegateImpl controller_delegate;
  VariableStorage variable_storage;
  VariableTimedDataService timed_data_service{variable_storage};
};
