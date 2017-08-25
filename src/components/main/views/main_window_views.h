#pragma once

#include <map>

#include "components/main/main_window.h"
#include "dialog_service.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/controls/toolbar.h"

namespace views {
class Menu;
}

class NativeMainWindow;
class ToolbarController;
class ViewManagerViews;

class MainWindowViews : public MainWindow,
                        public views::View,
                        public DialogServiceViews,
                        private views::Toolbar::Controller {
 public:
  explicit MainWindowViews(MainWindowContext&& context);
  ~MainWindowViews();

  gfx::NativeView GetWindowHandle() const;

  base::string16 GetWindowTitle() const;

  // MainWindow
  virtual void SetWindowFlashing(bool flashing) override;
  virtual void UpdateToolbarPosition() override;
  virtual DialogService& GetDialogService() override { return *this; }

  // views::View
  virtual void Layout() override;
  virtual bool ExecuteWindowsCommand(int command_id) override;

  // DialogServiceViews
  virtual DialogParentType GetParentView() override;

 protected:
  // MainWindow
  virtual void UpdateTitle() override;
  virtual void OnSelectionChanged() override;

  // ViewManagerDelegate
  virtual void OnShowTabPopupMenu(OpenedView& view, const gfx::Point& point) override;

 private:
  void LoadAccelerators();

  // views::Toolbar::Controller
  virtual void OnExecuteToolbarCommand(views::Toolbar& sender, unsigned command_id) override;

  // ui::AcceleratorTarget
  virtual bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  virtual bool CanHandleAccelerators() const override;

  std::unique_ptr<ViewManagerViews> view_manager_;

  NativeMainWindow* main_window_ = nullptr;

  std::unique_ptr<views::Toolbar> toolbar_;
  std::unique_ptr<ToolbarController> toolbar_controller_;

  // A mapping between accelerators and commands.
  std::map<ui::Accelerator, int> accelerator_table_; 
};
