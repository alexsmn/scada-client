#pragma once

#include "aui/wt/dialog_service_impl_wt.h"
#include "main_window/base_main_window.h"

namespace Wt {
class WContainerWidget;
}

class MainMenuController;
class DialogServiceImplWt;
class ToolbarController;
class ViewManagerWt;

class MainWindow final : public BaseMainWindow {
 public:
  MainWindow(Wt::WContainerWidget& parent, MainWindowContext&& context);
  ~MainWindow();

  // BaseMainWindow
  virtual DialogService& GetDialogService() override;
  virtual void SetWindowFlashing(bool flashing) override {}
  virtual void OnSelectionChanged() override;
  virtual void UpdateTitle() override {}
  virtual void SetToolbarPosition(unsigned position) override {}
  virtual void OnShowTabPopupMenu(OpenedView& view,
                                  const aui::Point& point) override {}
  virtual void ShowPopupMenu(aui::MenuModel* merge_menu,
                             unsigned resource_id,
                             const aui::Point& point,
                             bool right_click) override;
  virtual std::unique_ptr<OpenedView> OnCreateView(
      WindowDefinition& def) override;

 private:
  Wt::WContainerWidget& parent_;
  DialogServiceImplWt dialog_service_;

  std::unique_ptr<MainMenuController> main_menu_controller_;

  std::unique_ptr<ToolbarController> toolbar_controller_;

  std::unique_ptr<ViewManagerWt> view_manager_;
};
