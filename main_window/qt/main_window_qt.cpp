#include "main_window/qt/main_window_qt.h"

#include "aui/models/menu_model.h"
#include "aui/models/simple_menu_model.h"
#include "aui/qt/client_utils_qt.h"
#include "base/strings/string_util.h"
#include "base/win/win_util2.h"
#include "client_utils.h"
#include "common_resources.h"
#include "controller/controller.h"
#include "controller/selection_model.h"
#include "controller/window_info.h"
#include "filesystem/file_cache.h"
#include "main_window/action_manager.h"
#include "main_window/main_commands.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view.h"
#include "main_window/qt/status_bar_controller.h"
#include "main_window/qt/view_manager_qt.h"
#include "main_window/selection_commands.h"
#include "main_window/simple_menu_command_handler.h"
#include "profile/profile.h"

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
                    aui::MenuModel& context_menu_model,
                    aui::SimpleMenuModel& menu_model,
                    std::vector<std::unique_ptr<aui::MenuModel>>& submenus) {
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
          std::make_unique<aui::SimpleMenuModel>(menu_model.delegate());
      BuildMenuModel(menu_info.hSubMenu, context_menu_model, *submenu_model,
                     submenus);
      menu_model.AddSubMenu(menu_info.wID, base::AsString16(title),
                            submenu_model.get());
      submenus.emplace_back(std::move(submenu_model));

    } else if (menu_info.fType & MFT_SEPARATOR) {
      menu_model.AddSeparator(aui::NORMAL_SEPARATOR);

    } else if (menu_info.fState & MFS_CHECKED) {
      menu_model.AddCheckItem(menu_info.wID, base::AsString16(title));

    } else if (menu_info.wID == ID_ITEM_COMMANDS) {
      menu_model.AddInplaceMenu(&context_menu_model);

    } else {
      menu_model.AddItem(menu_info.wID, base::AsString16(title));
    }
  }
}

}  // namespace

MainWindowQt::MainWindowQt(MainWindowContext&& context)
    : MainWindow{std::move(context), dialog_service_} {
  auto& prefs = GetPrefs();
  auto bounds = prefs.bounds;
  if (bounds.empty()) {
    auto desktop_bounds = QDesktopWidget{}.availableGeometry(this);
    bounds = {desktop_bounds.left() + desktop_bounds.width() / 8,
              desktop_bounds.top() + desktop_bounds.height() / 8,
              desktop_bounds.width() * 3 / 4, desktop_bounds.height() * 3 / 4};
  }

  setGeometry(bounds.x, bounds.y, bounds.width, bounds.height);

  view_manager_ = std::make_unique<ViewManagerQt>(
      *this, *static_cast<ViewManagerDelegate*>(this));

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
      QString::fromStdU16String(FormatHostName(connection_info_provider_()));
  QString page =
      QString::fromStdU16String(view_manager_->current_page().GetTitle());
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
        QString::fromStdU16String(main_menu_model_->GetLabelAt(i)));
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
        QString::fromStdU16String(action_info->GetShortTitle()), this);
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
          auto title = GetCommandCategoryTitle(action_info->category_);
          auto text = QString::fromUtf16(title.data(), title.size());
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
                                      const aui::Point& point) {
  QMenu menu;
  BuildMenu(menu, *tab_popup_menu_);
  menu.exec(point);
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
      action.setText(QString::fromStdU16String(a->GetTitle()));
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
  prefs.bounds = {g.x(), g.y(), g.width(), g.height()};
  prefs.maximized = isMaximized();

  // ModusView-s must be destroyed before MainWindowQt destruction, to avoid an
  // exception of unknown nature.
  BeforeClose();

  main_window_manager_.OnMainWindowClosed(window_id_);

  QMainWindow::closeEvent(event);
}

void MainWindowQt::ShowPopupMenu(aui::MenuModel* merge_menu,
                                 unsigned resource_id,
                                 const aui::Point& point,
                                 bool right_click) {
  if (resource_id == 0) {
    QMenu menu;
    if (merge_menu && merge_menu->GetItemCount() != 0) {
      BuildMenu(menu, *merge_menu);
      menu.addSeparator();
    }
    BuildMenu(menu, *context_menu_model_);
    menu.exec(point);
    return;
  }

  SimpleMenuCommandHandler command_handler{commands()};
  aui::SimpleMenuModel menu_model{&command_handler};
  std::vector<std::unique_ptr<aui::MenuModel>> submenus;

  {
    CMenu resource_menu;
    resource_menu.LoadMenu(resource_id);
    BuildMenuModel(resource_menu.GetSubMenu(0), *context_menu_model_,
                   menu_model, submenus);
  }

  QMenu menu;
  // TODO: Combine with the same above.
  if (merge_menu && merge_menu->GetItemCount() != 0) {
    BuildMenu(menu, *merge_menu);
    menu.addSeparator();
  }
  BuildMenu(menu, menu_model);
  menu.exec(point);
}
