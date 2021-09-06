#pragma once

#include "components/main/main_window.h"
#include "wt/dialog_service_impl_wt.h"

class MainMenuController;
class DialogServiceImplWt;
class StatusBarController;
class ToolbarController;
class ViewManagerWt;

class MainWindowWt final : public MainWindow {
 public:
  MainWindowWt(Wt::WContainerWidget& parent, MainWindowContext&& context);
  ~MainWindowWt();

  // MainWindow
  virtual DialogService& GetDialogService() override;
  virtual void SetWindowFlashing(bool flashing) override {}
  virtual void OnSelectionChanged() override;
  virtual void UpdateTitle() override {}
  virtual void SetToolbarPosition(unsigned position) override {}
  virtual void OnShowTabPopupMenu(OpenedView& view,
                                  const gfx::Point& point) override {}

 private:
  Wt::WContainerWidget& parent_;
  DialogServiceImplWt dialog_service_;

  std::unique_ptr<MainMenuController> main_menu_controller_;

  std::unique_ptr<ToolbarController> toolbar_controller_;

  std::unique_ptr<ViewManagerWt> view_manager_;
};
