#include "client/components/main/views/main_menu_model.h"

#include "client/views/client_utils_views.h"
#include "client/common_resources.h"
#include "client/views/context_menu_model.h"
#include "client/components/main/opened_view.h"
#include "client/window_info.h"
#include "client/services/favourites.h"
#include "client/components/main/views/main_window_views.h"
#include "client/window_definition.h"

const unsigned kTableTypes[] = { ID_TABLE_VIEW, ID_SHEET_VIEW, ID_TIMED_DATA_VIEW, 0 };
const unsigned kGraphTypes[] = { ID_GRAPH_VIEW, 0 };

// OpenMenuModel

OpenMenuModel::OpenMenuModel(ui::SimpleMenuModel::Delegate* delegate,
                             const unsigned view_types[],
                             Favourites& favourites)
    : ui::SimpleMenuModel(delegate),
      view_types_(view_types),
      favourites_(favourites),
      first_favourite_(-1) {
}

void OpenMenuModel::MenuWillShow() {
  Rebuild();
}

void OpenMenuModel::FillFavourites() {
  if (const Page* favourites_folder = favourites_.GetFolder()) {
    for (int i = 0; i != favourites_folder->GetWindowCount(); ++i) {
      const WindowDefinition& win = favourites_folder->GetWindow(i);
      
      bool found = false;
      for (int i = 0; view_types_[i]; ++i) {
        if (win.window_info() && win.window_info()->command_id == view_types_[i]) {
          found = true;
          break;
        }
      }

      if (found)
        favourites_vector_.push_back(&win);
    }
  }
}

void OpenMenuModel::Rebuild() {
  if (!favourites_vector_.empty()) {
    Clear();
    favourites_vector_.clear();
    first_favourite_ = -1;
  }
 
  FillFavourites();
  if (favourites_vector_.empty())
    return;

  first_favourite_ = GetItemCount();
  AddSeparator(ui::NORMAL_SEPARATOR);
  for (auto* win : favourites_vector_)
    AddItem(0, win->GetTitle());
}

void OpenMenuModel::ActivatedAt(int index) {
  /*if (first_favourite_ != -1 && index >= first_favourite_ + 1 &&
      index < first_favourite_ + 1 + static_cast<int>(favourites_.size())) {
    int ix = index - first_favourite_ - 1;
    g_main_view->OpenView(*favourites_[ix], true);
  } else {
    ui::SimpleMenuModel::ActivatedAt(index);
  }*/
}

// MainMenuModel

MainMenuModel::MainMenuModel(MainWindowViews& main_view, ActionManager& action_manager, Favourites& favourites)
    : ui::SimpleMenuModel(this),
      main_view_(main_view),
      action_manager_(action_manager),
      favourites_(favourites),
      more_submenu_(this) {
  Rebuild();
}

void MainMenuModel::Rebuild() {
  table_submenu_.reset(new OpenMenuModel(this, kTableTypes, favourites_));
  table_submenu_->AddItem(ID_TABLE_VIEW, L"Новая таблица");
  table_submenu_->AddItem(ID_SHEET_VIEW, L"Новая п-таблица");
  table_submenu_->AddItem(ID_TIMED_DATA_VIEW, L"Новая таблица значений");
  AddSubMenu(0, L"Таблица", table_submenu_.get());

  graph_submenu_.reset(new OpenMenuModel(this, kGraphTypes, favourites_));
  graph_submenu_->AddItem(ID_GRAPH_VIEW, L"Новый");
  AddSubMenu(0, L"График", graph_submenu_.get());

  context_menu_ = std::make_unique<ContextMenuModel>(&main_view_, action_manager_);
  AddSubMenu(0, L"Объект", context_menu_.get());
  
  more_submenu_.AddItem(ID_OBJECT_VIEW, L"Объекты");
  more_submenu_.AddItem(ID_EVENT_VIEW, L"События");
  more_submenu_.AddItem(ID_FAVOURITES_VIEW, L"Избранное");
  more_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  more_submenu_.AddItem(ID_PORTFOLIO_VIEW, L"Портфолио");
  more_submenu_.AddItem(ID_HARDWARE_VIEW, L"Оборудование");
  more_submenu_.AddItem(ID_EVENT_JOURNAL_VIEW, L"Журнал событий");
  more_submenu_.AddItem(ID_STATISTICS_VIEW, L"Статус");
  more_submenu_.AddItem(ID_TS_FORMATS_VIEW, L"Форматы");
  more_submenu_.AddItem(ID_SIMULATION_ITEMS_VIEW, L"Эмулируемые сигналы");
  more_submenu_.AddItem(ID_USERS_VIEW, L"Пользователи");
  AddSubMenu(0, L"Далее", &more_submenu_);
}

void MainMenuModel::ExecuteCommand(int command_id) {
  main_view_.ExecuteWindowsCommand(command_id);
}
