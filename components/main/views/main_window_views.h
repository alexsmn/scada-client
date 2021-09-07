#pragma once

#include "components/main/main_window.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/controls/toolbar.h"
#include "views/dialog_service_impl_views.h"

class NativeMainWindow;
class ToolbarController;
class ViewManagerViews;

class MainWindowViews final : public MainWindow,
                              public views::View,
                              private views::Toolbar::Controller {
 public:
  explicit MainWindowViews(MainWindowContext&& context);
  ~MainWindowViews();

  gfx::NativeView GetWindowHandle() const;

  std::wstring GetWindowTitle() const;

  // views::View
  virtual DialogService& GetDialogService() override { return dialog_service_; }
  virtual void Layout() override;
  virtual bool ExecuteWindowsCommand(int command_id) override;

 protected:
  // MainWindow
  virtual void UpdateTitle() override;
  virtual void SetWindowFlashing(bool flashing) override;
  virtual void OnSelectionChanged() override;
  virtual void SetToolbarPosition(unsigned position) override;
  virtual void ShowPopupMenu(unsigned resource_id,
                             const gfx::Point& point,
                             bool right_click) override;

  // ViewManagerDelegate
  virtual void OnShowTabPopupMenu(OpenedView& view,
                                  const gfx::Point& point) override;

 private:
  void CreateToolbar();
  void LoadAccelerators();

  // views::Toolbar::Controller
  virtual void OnExecuteToolbarCommand(views::Toolbar& sender,
                                       unsigned command_id) override;

  // ui::AcceleratorTarget
  virtual bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  virtual bool CanHandleAccelerators() const override;

  std::unique_ptr<ViewManagerViews> view_manager_;

  NativeMainWindow* main_window_ = nullptr;

  std::unique_ptr<views::Toolbar> toolbar_;
  std::unique_ptr<ToolbarController> toolbar_controller_;

  // A mapping between accelerators and commands.
  std::map<ui::Accelerator, int> accelerator_table_;

  DialogServiceImplViews dialog_service_;
};
