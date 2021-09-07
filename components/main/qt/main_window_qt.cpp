#include "components/main/qt/main_window_qt.h"

#include "base/win/win_util2.h"
#include "client_utils.h"
#include "common_resources.h"
#include "components/main/action_manager.h"
#include "components/main/main_commands.h"
#include "components/main/main_window_manager.h"
#include "components/main/opened_view.h"
#include "components/main/qt/status_bar_controller.h"
#include "components/main/qt/view_manager_qt.h"
#include "components/main/selection_commands.h"
#include "controller.h"
#include "qt/client_utils_qt.h"
#include "selection_model.h"
#include "services/file_cache.h"
#include "services/profile.h"
#include "simple_menu_command_handler.h"
#include "ui/base/models/menu_model.h"
#include "ui/base/models/simple_menu_model.h"
#include "window_info.h"

#include <QAction>
#include <QApplication>
#include <QDesktopWidget>
#include <QDockWidget>
#include <QEvent>
#include <QLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QStyleFactory>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>

#include <atlbase.h>

#include <atlapp.h>

#include <atluser.h>

namespace {

inline QKeySequence ToQKeySequence(const Shortcut& shortcut) {
  return QKeySequence{static_cast<int>(shortcut.key_code()) +
                      static_cast<int>(shortcut.modifiers())};
}

void BuildMenuModel(CMenuHandle menu_handle,
                    ui::MenuModel& context_menu_model,
                    ui::SimpleMenuModel& menu_model,
                    std::vector<std::unique_ptr<ui::MenuModel>>& submenus) {
  for (int i = 0; i < menu_handle.GetMenuItemCount(); ++i) {
    wchar_t title[64] = {};

    CMenuItemInfo menu_info;
    menu_info.fMask |=
        MIIM_FTYPE | MIIM_ID | MIIM_STRING | MIIM_SUBMENU | MIIM_STATE;
    menu_info.cch = std::size(title);
    menu_info.dwTypeData = title;
    menu_handle.GetMenuItemInfo(i, TRUE, &menu_info);

    if (menu_info.hSubMenu) {
      auto submenu_model =
          std::make_unique<ui::SimpleMenuModel>(menu_model.delegate());
      BuildMenuModel(menu_info.hSubMenu, context_menu_model, *submenu_model,
                     submenus);
      menu_model.AddSubMenu(menu_info.wID, title, submenu_model.get());
      submenus.emplace_back(std::move(submenu_model));

    } else if (menu_info.fType & MFT_SEPARATOR) {
      menu_model.AddSeparator(ui::NORMAL_SEPARATOR);

    } else if (menu_info.fState & MFS_CHECKED) {
      menu_model.AddCheckItem(menu_info.wID, title);

    } else if (menu_info.wID == ID_ITEM_COMMANDS) {
      menu_model.AddInplaceMenu(&context_menu_model);

    } else {
      menu_model.AddItem(menu_info.wID, title);
    }
  }
}

}  // namespace

MainWindowQt::MainWindowQt(MainWindowContext&& context)
    : MainWindow{std::move(context), dialog_service_} {
  auto& prefs = GetPrefs();
  auto bounds = prefs.bounds;
  if (bounds.IsEmpty()) {
    auto desktop_bounds = QDesktopWidget{}.availableGeometry(this);
    bounds = {desktop_bounds.left() + desktop_bounds.width() / 8,
              desktop_bounds.top() + desktop_bounds.height() / 8,
              desktop_bounds.width() * 3 / 4, desktop_bounds.height() * 3 / 4};
  }

  setGeometry(bounds.x(), bounds.y(), bounds.width(), bounds.height());

  view_manager_.reset(new ViewManagerQt{*this, *this});

  dialog_service_.parent_widget = this;

  CreateToolbar();

  // Must show window before loading layout to let |restoreState()| work
  // correctly.
  if (prefs.maximized)
    showMaximized();
  else
    show();

  Init(*view_manager_);

  action_manager_.Subscribe(*this);
}

MainWindowQt::~MainWindowQt() {
  action_manager_.Unsubscribe(*this);
  view_manager_.reset();
}

void MainWindowQt::UpdateTitle() {
  QString server =
      QString::fromStdWString(FormatHostName(connection_info_provider_()));
  QString page =
      QString::fromStdWString(view_manager_->current_page().GetTitle());
  QString title = tr("%1 (Server: %2)").arg(page).arg(server);
  setWindowTitle(title);
}

void MainWindowQt::CreateToolbar() {
  /*auto* settings_menu = new QMenu(tr("Settings"), this);
  auto* style_menu = new QMenu(tr("Style"), this);
  for (auto& style : QStyleFactory::keys())
    style_menu->addAction(style,
                          [this, style] { QApplication::setStyle(style); });
  settings_menu->addMenu(style_menu);*/

  main_menu_model_ = main_menu_factory_(*this, dialog_service_, *view_manager_,
                                        *commands_, *context_menu_model_);

  auto* menu_bar = new QMenuBar(this);
  setMenuBar(menu_bar);

  for (int i = 0; i < main_menu_model_->GetItemCount(); ++i) {
    auto* submenu = menu_bar->addMenu(
        QString::fromStdWString(main_menu_model_->GetLabelAt(i)));
    auto* submenu_model = main_menu_model_->GetSubmenuModelAt(i);
    assert(submenu_model);
    QObject::connect(submenu, &QMenu::aboutToShow, this,
                     [this, submenu, submenu_model] {
                       submenu->clear();
                       BuildMenu(*submenu, *submenu_model);
                     });
  }

  setStatusBar(new QStatusBar(this));
  status_bar_controller_ =
      std::make_unique<StatusBarController>(*statusBar(), *status_bar_model_);

  for (auto* action_info : action_manager_.actions()) {
    bool collapsible = !CanExpandCommandCategory(action_info->category_);
    auto* action = new QAction(
        QString::fromStdWString(action_info->GetShortTitle()), this);
    action->setPriority(collapsible ? QAction::LowPriority
                                    : QAction::NormalPriority);
    action->setVisible(false);
    if (action_info->image_id() != 0)
      action->setIcon(QIcon(LoadPixmap(action_info->image_id())));
    action->setCheckable(action_info->checkable());
    if (action_info->shortcut_.has_value())
      action->setShortcut(ToQKeySequence(*action_info->shortcut_));
    auto command_id = action_info->command_id();
    QObject::connect(action, &QAction::triggered,
                     [this, command_id](bool checked) {
                       auto* handler = commands_->GetCommandHandler(command_id);
                       if (handler && handler->IsCommandEnabled(command_id))
                         handler->ExecuteCommand(command_id);
                     });
    action_map_.emplace(action_info->command_id(), action);
    action_command_ids_.emplace(action, action_info->command_id());
  }

  toolbar_ = new QToolBar(this);
  toolbar_->setWindowTitle(tr("Toolbar"));

  {
    // Action order is important.
    int last_category = -1;
    for (auto* action_info : action_manager_.actions()) {
      auto* action = FindAction(action_info->command_id());
      if (CanExpandCommandCategory(action_info->category_)) {
        toolbar_->addAction(action);
        if (last_category != -1 && last_category != action_info->category_) {
          toolbar_->addSeparator();
        }
      } else {
        auto& category_action = category_actions_[action_info->category_];
        if (!category_action.menu) {
          auto* button = new QToolButton(toolbar_);
          auto* menu = new QMenu(this);
          auto text = QString::fromWCharArray(
              GetCommandCategoryTitle(action_info->category_));
          button->setMenu(menu);
          button->setPopupMode(QToolButton::InstantPopup);
          button->setText(text);
          category_action.menu = menu;
          category_action.toolbar_action = toolbar_->addWidget(button);
          connect(menu, &QMenu::aboutToShow,
                  [this, menu] { UpdateMenuActions(*menu); });
        }
        category_action.menu->addAction(action);
      }
      last_category = action_info->category_;
    }
  }

  addToolBar(Qt::TopToolBarArea, toolbar_);
}

void MainWindowQt::SetWindowFlashing(bool flashing) {}

void MainWindowQt::OnSelectionChanged() {
  for (auto& p : action_map_)
    UpdateAction(*p.second, p.first, ActionChangeMask::AllButTitle);

  for (auto& p : category_actions_) {
    bool visible = false;
    for (auto* action : p.second.menu->actions()) {
      if (action->isVisible()) {
        visible = true;
        break;
      }
    }
    p.second.toolbar_action->setVisible(visible);
  }

  bool adjucent_separator = false;
  for (auto* toolbar_action : toolbar_->actions()) {
    if (toolbar_action->isSeparator()) {
      toolbar_action->setVisible(!adjucent_separator);
      adjucent_separator = true;
    } else if (toolbar_action->isVisible()) {
      adjucent_separator = false;
    }
  }
}

void MainWindowQt::SetToolbarPosition(unsigned position) {}

void MainWindowQt::OnShowTabPopupMenu(OpenedView& view,
                                      const gfx::Point& point) {
  QMenu menu;
  BuildMenu(menu, *tab_popup_menu_);
  menu.exec({point.x(), point.y()});
}

QAction* MainWindowQt::FindAction(unsigned command_id) {
  auto i = action_map_.find(command_id);
  return i == action_map_.end() ? nullptr : i->second;
}

void MainWindowQt::OnActionChanged(Action& action,
                                   ActionChangeMask change_mask) {
  auto i = action_map_.find(action.command_id());
  if (i != action_map_.end())
    UpdateAction(*i->second, i->first, change_mask);
}

void MainWindowQt::UpdateAction(QAction& action,
                                unsigned command_id,
                                ActionChangeMask change_mask) {
  if (static_cast<unsigned>(change_mask) &
      static_cast<unsigned>(ActionChangeMask::Title)) {
    if (auto* a = action_manager_.FindAction(command_id))
      action.setText(QString::fromStdWString(a->GetTitle()));
  }

  auto* handler = commands_->GetCommandHandler(command_id);
  action.setVisible(!!handler);
  if (handler) {
    bool enabled = handler->IsCommandEnabled(command_id);
    action.setEnabled(enabled);
    if (enabled)
      action.setChecked(handler->IsCommandChecked(command_id));
  }
}

void MainWindowQt::UpdateMenuActions(QMenu& menu) {
  for (auto* action : menu.actions()) {
    auto i = action_command_ids_.find(action);
    if (i != action_command_ids_.end())
      UpdateAction(*action, i->second, ActionChangeMask::All);
  }
}

void MainWindowQt::closeEvent(QCloseEvent* event) {
  auto& prefs = GetPrefs();
  const auto& g = geometry();
  prefs.bounds = gfx::Rect{g.x(), g.y(), g.width(), g.height()};
  prefs.maximized = isMaximized();

  // ModusView-s must be destroyed before MainWindowQt destruction, to avoid an
  // exception of unknown nature.
  BeforeClose();

  main_window_manager_.OnMainWindowClosed(window_id_);

  QMainWindow::closeEvent(event);
}

void MainWindowQt::ShowPopupMenu(unsigned resource_id,
                                 const UiPoint& point,
                                 bool right_click) {
  if (resource_id == 0) {
    QMenu menu;
    BuildMenu(menu, *context_menu_model_);
    menu.exec(point);
    return;
  }

  SimpleMenuCommandHandler command_handler{commands()};
  ui::SimpleMenuModel menu_model{&command_handler};
  std::vector<std::unique_ptr<ui::MenuModel>> submenus;

  {
    CMenu resource_menu;
    resource_menu.LoadMenu(resource_id);
    BuildMenuModel(resource_menu.GetSubMenu(0), *context_menu_model_,
                   menu_model, submenus);
  }

  QMenu menu;
  BuildMenu(menu, menu_model);
  menu.exec(point);
}
