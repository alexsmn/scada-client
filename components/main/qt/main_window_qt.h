#pragma once

#include "components/main/main_window.h"
#include "services/dialog_service.h"

#include <QMainWindow>

class ClientApplicationQt;
class QAction;
class QMenu;
class QWidget;
class ViewManagerQt;
enum CommandCategory;

class MainWindowQt : public QMainWindow,
                     public DialogServiceQt,
                     public MainWindow {
  Q_OBJECT

 public:
  MainWindowQt(ClientApplicationQt& app, MainWindowContext&& context);
  ~MainWindowQt();

  QMenu& context_menu() const { return *context_menu_; }

  // MainWindow
  virtual void SetWindowFlashing(bool flashing) override;
  virtual void UpdateToolbarPosition() override;
  virtual DialogService& GetDialogService() override { return *this; }

  // DialogServiceQt
  virtual DialogParentType GetParentView() override;

 protected:
  // MainWindow
  virtual void UpdateTitle() override;
  virtual void OnSelectionChanged() override;

  // ViewManagerDelegate
  virtual void OnShowTabPopupMenu(OpenedView& view, const gfx::Point& point) override;

 private:
  void CreateToolbar();

  QAction* FindAction(unsigned command_id);

  void FillDisplayMenu();

  std::unique_ptr<ViewManagerQt> view_manager_;

  std::map<unsigned /*command_id*/, QAction*> action_map_;

  QToolBar* toolbar_ = nullptr;

  struct CategoryData {
    CategoryData() : menu(nullptr), toolbar_action(nullptr), context_menu_action(nullptr) {}
    QMenu* menu;
    QAction* toolbar_action;
    QAction* context_menu_action;
  };

  std::map<CommandCategory, CategoryData> category_actions_;

  QMenu* display_menu_;
  QMenu* context_menu_;
};
