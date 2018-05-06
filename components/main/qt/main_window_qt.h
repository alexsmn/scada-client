#pragma once

#include "components/main/main_window.h"
#include "qt/dialog_service_impl_qt.h"

#include <QMainWindow>

class QAction;
class QMenu;
class QWidget;
class ViewManagerQt;

class MainWindowQt final : public MainWindow, public QMainWindow {
 public:
  explicit MainWindowQt(MainWindowContext&& context);
  ~MainWindowQt();

  // MainWindow
  virtual DialogService& GetDialogService() override { return dialog_service_; }

 protected:
  // MainWindow
  virtual void UpdateTitle() override;
  virtual void SetWindowFlashing(bool flashing) override;
  virtual void OnSelectionChanged() override;
  virtual void UpdateToolbarPosition() override;

  // ViewManagerDelegate
  virtual void OnShowTabPopupMenu(OpenedView& view,
                                  const gfx::Point& point) override;

 private:
  void CreateToolbar();

  QAction* FindAction(unsigned command_id);

  std::unique_ptr<ViewManagerQt> view_manager_;

  std::map<unsigned /*command_id*/, QAction*> action_map_;

  QToolBar* toolbar_ = nullptr;

  struct CategoryData {
    QMenu* menu = nullptr;
    QAction* toolbar_action = nullptr;
  };

  std::map<CommandCategory, CategoryData> category_actions_;

  DialogServiceImplQt dialog_service_;

  std::unique_ptr<ui::MenuModel> main_menu_model_;
};
