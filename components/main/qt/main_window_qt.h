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

  QMenu& context_menu() const { return *context_menu_; }

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

  void FillDisplayMenu();

  std::unique_ptr<ViewManagerQt> view_manager_;

  std::map<unsigned /*command_id*/, QAction*> action_map_;

  QToolBar* toolbar_ = nullptr;

  struct CategoryData {
    CategoryData()
        : menu(nullptr),
          toolbar_action(nullptr),
          context_menu_action(nullptr) {}
    QMenu* menu;
    QAction* toolbar_action;
    QAction* context_menu_action;
  };

  std::map<CommandCategory, CategoryData> category_actions_;

  QMenu* display_menu_ = nullptr;
  QMenu* context_menu_ = nullptr;

  DialogServiceImplQt dialog_service_;
};
