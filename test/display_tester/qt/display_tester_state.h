#pragma once

#include "aui/qt/dialog_service_impl_qt.h"
#include "base/check.h"
#include "controller/controller_delegate.h"
#include "modules/write/write_service.h"
#include "test/display_tester/qt/variable_timed_data_service.h"

class ControllerDelegateImpl final : public ControllerDelegate {
 public:
  virtual void SetTitle(std::u16string_view title) override {}
  virtual void ShowPopupMenu(aui::MenuModel* merge_menu,
                             unsigned resource_id,
                             const aui::Point& point,
                             bool right_click) override {}
  virtual void SetModified(bool modified) override {}
  virtual void Close() override {}
  virtual void OpenView(const WindowDefinition& def) override {}
  virtual void ExecuteDefaultNodeCommand(const NodeRef& node) override {}
  virtual ContentsModel* GetActiveContentsModel() override { return nullptr; }
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeContentsChanged(const ContentsChangedCallback& callback) override {
    return {};
  }
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeContainedItemChanged(
      const ContainedItemChangedCallback& callback) override {
    return {};
  }
  virtual void Focus() override {}
};

class WriteServiceImpl final : public WriteService {
 public:
  virtual void ExecuteWriteDialog(DialogService& dialog_service,
                                  const scada::NodeId& node_id,
                                  bool manual) override {
    base::NotReached();
  }
};

struct DisplayTesterState {
  ControllerDelegateImpl controller_delegate;
  VariableStorage variable_storage;
  VariableTimedDataService timed_data_service{variable_storage};
  DialogServiceImplQt dialog_service;
  WriteServiceImpl write_service;
};
