#include "components/main/qt/main_window_qt.h"

#include "client_utils.h"
#include "common_resources.h"
#include "components/main/action_manager.h"
#include "components/main/main_commands.h"
#include "components/main/main_window_manager.h"
#include "components/main/opened_view.h"
#include "components/main/qt/view_manager_qt.h"
#include "components/main/selection_commands.h"
#include "controller.h"
#include "selection_model.h"
#include "services/file_cache.h"
#include "services/profile.h"
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

  CreateToolbar();

  view_manager_.reset(new ViewManagerQt{*this, *this});

  dialog_service_.parent_widget = this;

  Init(*view_manager_);

  show();
}

MainWindowQt::~MainWindowQt() {
  BeforeClose();
  view_manager_.reset();
}

void MainWindowQt::UpdateTitle() {}

void MainWindowQt::CreateToolbar() {
  context_menu_ = new QMenu(tr("Context"), this);

  display_menu_ = new QMenu(tr("Display"), this);
  FillDisplayMenu();

  auto* graph_menu = new QMenu(tr("Graph"), this);
  auto* table_menu = new QMenu(tr("Table"), this);
  auto* rest_menu = new QMenu(tr("More"), this);
  auto* page_menu = new QMenu(tr("Page"), this);

  auto* settings_menu = new QMenu(tr("Settings"), this);
  auto* style_menu = new QMenu(tr("Style"), this);
  for (auto& style : QStyleFactory::keys())
    style_menu->addAction(style,
                          [this, style] { QApplication::setStyle(style); });
  settings_menu->addMenu(style_menu);

  auto* menu_bar = new QMenuBar;
  setMenuBar(menu_bar);
  menu_bar->addMenu(display_menu_);
  menu_bar->addMenu(graph_menu);
  menu_bar->addMenu(table_menu);
  menu_bar->addMenu(context_menu_);
  menu_bar->addMenu(rest_menu);
  menu_bar->addSeparator();
  menu_bar->addMenu(page_menu);
  menu_bar->addMenu(settings_menu);
  menu_bar->addMenu(tr("Help"));

  setStatusBar(new QStatusBar);

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
  addToolBar(Qt::TopToolBarArea, toolbar_);

  {
    // Action order is important.
    int last_category = -1;
    for (auto* action_info : action_manager_.actions()) {
      auto* action = FindAction(action_info->command_id());
      if (CanExpandCommandCategory(action_info->category_)) {
        toolbar_->addAction(action);
        context_menu_->addAction(action);
        if (last_category != -1 && last_category != action_info->category_) {
          toolbar_->addSeparator();
          context_menu_->addSeparator();
        }
      } else {
        auto& category_action = category_actions_[action_info->category_];
        if (!category_action.menu) {
          auto* button = new QToolButton();
          auto* menu = new QMenu(this);
          auto text = QString::fromWCharArray(
              GetCommandCategoryTitle(action_info->category_));
          button->setMenu(menu);
          button->setPopupMode(QToolButton::InstantPopup);
          button->setText(text);
          category_action.menu = menu;
          category_action.toolbar_action = toolbar_->addWidget(button);
          category_action.context_menu_action = context_menu_->addMenu(menu);
          category_action.context_menu_action->setText(text);
        }
        category_action.menu->addAction(action);
      }
      last_category = action_info->category_;
    }
  }

  for (int i = 0; i < VIEW_TYPE_COUNT; ++i) {
    auto& window_info = g_window_infos[i];
    auto* action =
        new QAction(QString::fromWCharArray(window_info.title), this);
    auto command_id = window_info.command_id;
    QObject::connect(action, &QAction::triggered,
                     [this, command_id](bool checked) {
                       auto* handler = commands_->GetCommandHandler(command_id);
                       if (handler && handler->IsCommandEnabled(command_id))
                         handler->ExecuteCommand(command_id);
                     });
    action_map_.emplace(command_id, action);
  }

  graph_menu->addAction(FindAction(ID_GRAPH_VIEW));

  table_menu->addAction(FindAction(ID_TABLE_VIEW));
  table_menu->addAction(FindAction(ID_SHEET_VIEW));
  table_menu->addAction(FindAction(ID_CELLS_VIEW));

  rest_menu->addAction(FindAction(ID_OBJECT_VIEW));
  rest_menu->addAction(FindAction(ID_EVENT_VIEW));
  rest_menu->addAction(FindAction(ID_FAVOURITES_VIEW));
  rest_menu->addAction(FindAction(ID_PORTFOLIO_VIEW));
  rest_menu->addAction(FindAction(ID_HARDWARE_VIEW));
  rest_menu->addAction(FindAction(ID_EVENT_JOURNAL_VIEW));
  rest_menu->addAction(FindAction(ID_STATISTICS_VIEW));
  rest_menu->addSeparator();
  rest_menu->addAction(FindAction(ID_TYPES_VIEW));
  rest_menu->addAction(FindAction(ID_TS_FORMATS_VIEW));
  rest_menu->addAction(FindAction(ID_SIMULATION_ITEMS_VIEW));
  rest_menu->addAction(FindAction(ID_USERS_VIEW));
  rest_menu->addAction(FindAction(ID_HISTORICAL_DB_VIEW));
  rest_menu->addSeparator();
  rest_menu->addAction(FindAction(ID_EXPORT_CONFIGURATION_TO_EXCEL));
  rest_menu->addAction(FindAction(ID_IMPORT_CONFIGURATION_FROM_EXCEL));

  page_menu->addAction(FindAction(ID_PAGE_NEW));
  page_menu->addAction(FindAction(ID_PAGE_DELETE));
  page_menu->addAction(FindAction(ID_PAGE_RENAME));
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
    p.second.context_menu_action->setVisible(visible);
  }
}

void MainWindowQt::UpdateToolbarPosition() {}

void MainWindowQt::OnShowTabPopupMenu(OpenedView& view,
                                      const gfx::Point& point) {}

void MainWindowQt::FillDisplayMenu() {
  display_menu_->clear();

  for (auto& entry : file_cache_.GetList(VIEW_TYPE_MODUS)) {
    auto* action =
        display_menu_->addAction(QString::fromStdWString(entry.title));
    auto path = entry.path;
    QObject::connect(action, &QAction::triggered, [this, path] {
      // find existing display
      auto* view = main_window_manager_.FindOpenedViewByFilePath(path);
      if (view) {
        view->Activate();
      } else {
        // add new window
        WindowDefinition def(GetWindowInfo(ID_MODUS_VIEW));
        def.path = path;
        OpenView(def, true);
      }
    });
  }
}

QAction* MainWindowQt::FindAction(unsigned command_id) {
  auto i = action_map_.find(command_id);
  return i == action_map_.end() ? nullptr : i->second;
}
