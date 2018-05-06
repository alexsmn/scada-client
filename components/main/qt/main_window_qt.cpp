#include "components/main/qt/main_window_qt.h"

#include "base/win/win_util2.h"
#include "client_utils.h"
#include "common_resources.h"
#include "components/main/action_manager.h"
#include "components/main/main_commands.h"
#include "components/main/main_window_manager.h"
#include "components/main/opened_view.h"
#include "components/main/qt/view_manager_qt.h"
#include "components/main/selection_commands.h"
#include "controller.h"
#include "qt/client_utils_qt.h"
#include "selection_model.h"
#include "services/file_cache.h"
#include "services/profile.h"
#include "ui/base/models/menu_model.h"
#include "window_info.h"

#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QEvent>
#include <QLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QStyleFactory>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>

namespace {

QPixmap LoadPixmap(unsigned resource_id) {
  HRSRC hres = FindResource(NULL, MAKEINTRESOURCE(resource_id), L"PNG");
  DWORD size = SizeofResource(NULL, hres);

  HGLOBAL resource = LoadResource(NULL, hres);

  LPVOID resource_data = LockResource(resource);

  QPixmap pixmap;
  pixmap.loadFromData(static_cast<const uchar*>(resource_data), size);
  return pixmap;
}

}  // namespace

MainWindowQt::MainWindowQt(MainWindowContext&& context)
    : MainWindow{std::move(context), dialog_service_} {
  setWindowTitle(tr("Telecontrol SCADA Client"));

  auto& prefs = GetPrefs();
  setGeometry(prefs.bounds.x(), prefs.bounds.y(), prefs.bounds.width(),
              prefs.bounds.height());

  view_manager_.reset(new ViewManagerQt{*this, *this});

  dialog_service_.parent_widget = this;

  CreateToolbar();

  Init(*view_manager_);

  show();
}

MainWindowQt::~MainWindowQt() {
  BeforeClose();
  view_manager_.reset();
}

void MainWindowQt::UpdateTitle() {}

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

  for (auto* action_info : action_manager_.actions()) {
    bool collapsible = !CanExpandCommandCategory(action_info->category_);
    auto* action = new QAction(
        QString::fromStdWString(action_info->GetShortTitle()), this);
    action->setPriority(collapsible ? QAction::LowPriority
                                    : QAction::NormalPriority);
    action->setVisible(false);
    if (action_info->image_id() != 0)
      action->setIcon(QIcon(LoadPixmap(action_info->image_id())));
    auto command_id = action_info->command_id();
    QObject::connect(action, &QAction::triggered,
                     [this, command_id](bool checked) {
                       auto* handler = commands_->GetCommandHandler(command_id);
                       if (handler && handler->IsCommandEnabled(command_id))
                         handler->ExecuteCommand(command_id);
                     });
    action_map_.emplace(action_info->command_id(), action);
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
  for (auto& p : action_map_) {
    auto command_id = p.first;
    auto* action = p.second;
    auto* handler = commands_->GetCommandHandler(p.first);
    action->setVisible(!!handler);
    if (handler) {
      bool enabled = handler->IsCommandEnabled(command_id);
      action->setEnabled(enabled);
      if (enabled)
        action->setChecked(handler->IsCommandChecked(command_id));
    }
  }

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
}

void MainWindowQt::SetToolbarPosition(unsigned position) {}

void MainWindowQt::OnShowTabPopupMenu(OpenedView& view,
                                      const gfx::Point& point) {}

QAction* MainWindowQt::FindAction(unsigned command_id) {
  auto i = action_map_.find(command_id);
  return i == action_map_.end() ? nullptr : i->second;
}
