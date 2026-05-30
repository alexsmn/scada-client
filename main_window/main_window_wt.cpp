#include "main_window/main_window_wt.h"

#include "main_window/main_menu/main_menu_controller_wt.h"
#include "main_window/toolbar_controller_wt.h"
#include "main_window/view_manager.h"
#include "controller/command_ui_registry.h"

#pragma warning(push)
#pragma warning(disable : 4251 4275)
#include <wt/WContainerWidget.h>
#include <wt/WVBoxLayout.h>
#pragma warning(pop)

// MainWindow

MainWindow::MainWindow(Wt::WContainerWidget& parent,
                       MainWindowContext&& context)
    : BaseMainWindow{std::move(context), dialog_service_}, parent_{parent} {
  auto* root_layout = parent.setLayout(std::make_unique<Wt::WVBoxLayout>());

  auto main_menu_model = main_menu_factory_(
      *this, dialog_service_, *view_manager_, *commands_, *context_menu_model_);
  main_menu_controller_ = std::make_unique<MainMenuController>(
      MainMenuControllerContext{std::move(main_menu_model)});
  root_layout->addWidget(main_menu_controller_->CreateWidget());

  ViewManagerDelegate& delegate = *this;
  toolbar_controller_ = std::make_unique<ToolbarController>(
      ToolbarControllerContext{executor_, ui_command_registry_.action_manager(),
                               *commands_});
  root_layout->addWidget(toolbar_controller_->CreateToolbar());

  view_manager_ = std::make_unique<ViewManager>(delegate);

  auto* root_layout_widget =
      root_layout->addWidget(std::make_unique<Wt::WContainerWidget>(), 1);
  root_layout_widget->setLayout(
      std::unique_ptr<Wt::WLayout>(&view_manager_->root_layout()));
  Init(*view_manager_);
}

MainWindow::~MainWindow() {
  view_manager_->ClosePage();
}

DialogService& MainWindow::GetDialogService() {
  return dialog_service_;
}

void MainWindow::OnSelectionChanged() {
  toolbar_controller_->OnSelectionChanged();
}

void MainWindow::ShowPopupMenu(aui::MenuModel* merge_menu,
                               unsigned resource_id,
                               const aui::Point& point,
                               bool right_click) {}

std::unique_ptr<OpenedView> MainWindow::OnCreateView(
    WindowDefinition& window_def) {
  return opened_view_factory_(*this, window_def);
}
