#pragma once

#include "aui/qt/dialog_service_impl_qt.h"
#include "main_window/action_manager.h"
#include "main_window/base_main_window.h"

#include <QMainWindow>
#include <boost/signals2/connection.hpp>

class QAction;
class QMenu;
class QWidget;
class StatusBarController;
class ViewManagerQt;

class MainWindow final : public QMainWindow,
                         public BaseMainWindow,
                         private ActionObserver {
  Q_OBJECT

 public:
  explicit MainWindow(MainWindowContext&& context);
  ~MainWindow();

  // BaseMainWindow
  virtual DialogService& GetDialogService() override { return dialog_service_; }
  virtual void SetWindowFlashing(bool flashing) override;
  virtual void ShowPopupMenu(aui::MenuModel* merge_menu,
                             unsigned resource_id,
                             const aui::Point& point,
                             bool right_click) override;

 protected:
  // BaseMainWindow
  virtual void UpdateTitle() override;
  virtual void OnSelectionChanged() override;
  virtual void SetToolbarPosition(unsigned position) override;
  virtual std::unique_ptr<OpenedView> OnCreateView(
      WindowDefinition& def) override;

  // ViewManagerDelegate
  virtual void OnShowTabPopupMenu(OpenedView& view,
                                  const aui::Point& point) override;

  // QWidget
  virtual void closeEvent(QCloseEvent* event) override;

 private:
  void CreateMenuBar();
  void CreateToolbar();
  void CreateStatusBar();

  QAction* FindAction(unsigned command_id);

  void UpdateAction(QAction& qaction,
                    unsigned command_id,
                    ActionChangeMask change_mask);
  void UpdateMenuActions(QMenu& menu);

  // ActionObserver
  virtual void OnActionChanged(Action& action,
                               ActionChangeMask change_mask) override;

  std::unique_ptr<ViewManagerQt> view_manager_;

  std::map<unsigned /*command_id*/, QAction*> action_map_;
  std::map<QAction*, unsigned /*command_id*/> action_command_ids_;

  QToolBar* toolbar_ = nullptr;

  struct CategoryData {
    QMenu* menu = nullptr;
    QAction* toolbar_action = nullptr;
  };

  std::map<CommandCategory, CategoryData> category_actions_;

  DialogServiceImplQt dialog_service_;

  std::unique_ptr<aui::MenuModel> main_menu_model_;

  std::unique_ptr<StatusBarController> status_bar_controller_;

  boost::signals2::scoped_connection change_profile_connection_;
};
