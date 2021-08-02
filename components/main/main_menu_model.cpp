#include "components/main/main_menu_model.h"

#include "base/command_line.h"
#include "command_handler.h"
#include "common_resources.h"
#include "components/favourites/favourites.h"
#include "components/main/context_menu_model.h"
#include "components/main/main_window.h"
#include "components/main/main_window_manager.h"
#include "components/main/opened_view.h"
#include "components/main/view_manager.h"
#include "services/dialog_service.h"
#include "services/file_cache.h"
#include "services/profile.h"
#include "window_definition.h"
#include "window_info.h"

#include <atlres.h>

#if defined(UI_QT)
#include <QApplication>
#include <QStyle>
#include <QStyleFactory>
#endif

const unsigned kTableTypes[] = {ID_TABLE_VIEW, ID_SHEET_VIEW,
                                ID_TIMED_DATA_VIEW, 0};
const unsigned kGraphTypes[] = {ID_GRAPH_VIEW, 0};

// DisplayMenuModel

DisplayMenuModel::DisplayMenuModel(const MainMenuContext& context)
    : ui::SimpleMenuModel{nullptr}, MainMenuContext{std::move(context)} {}

void DisplayMenuModel::MenuWillShow() {
  Clear();
  items_.clear();

  AddItems(ID_MODUS_VIEW);
  AddItems(ID_VIDICON_DISPLAY_VIEW);
}

void DisplayMenuModel::ActivatedAt(int index) {
  auto& item = items_[index];
  // find existing display
  if (auto* view = main_window_manager_.FindOpenedViewByFilePath(item.path)) {
    view->Activate();
  } else {
    // add new window
    WindowDefinition def(GetWindowInfo(item.command_id));
    def.path = item.path;
    main_window_.OpenView(def, true);
  }
}

bool DisplayMenuModel::IsEnabledAt(int index) const {
  return !items_.empty();
}

void DisplayMenuModel::AddItems(unsigned command_id) {
  for (auto& entry : file_cache_.GetList(command_id)) {
    AddItem(0, entry.title);
    items_.push_back(Item{command_id, entry.path});
  }

  if (items_.empty())
    AddItem(0, L"<Нет схем>");
}

// FavouritesMenuModel

FavouritesMenuModel::FavouritesMenuModel(const unsigned view_types[],
                                         const MainMenuContext& context)
    : MainMenuContext{std::move(context)},
      ui::SimpleMenuModel{nullptr},
      view_types_{view_types} {}

void FavouritesMenuModel::MenuWillShow() {
  Clear();
  windows_.clear();

  if (const Page* favourites_folder = favourites_.GetFolder()) {
    for (int i = 0; i != favourites_folder->GetWindowCount(); ++i) {
      const WindowDefinition& win = favourites_folder->GetWindow(i);

      bool found = false;
      for (int j = 0; view_types_[j]; ++j) {
        if (win.window_info().command_id == view_types_[j]) {
          found = true;
          break;
        }
      }

      if (found) {
        AddItem(0, win.GetTitle());
        windows_.push_back(&win);
      }
    }
  }

  if (windows_.empty())
    AddItem(0, L"<Нет избранного>");
}

void FavouritesMenuModel::ActivatedAt(int index) {
  main_window_.OpenView(*windows_[index], true);
}

bool FavouritesMenuModel::IsEnabledAt(int index) const {
  return !windows_.empty();
}

// PageMenuModel

PageMenuModel::PageMenuModel(const MainMenuContext& context)
    : MainMenuContext{context}, ui::SimpleMenuModel{nullptr} {}

void PageMenuModel::MenuWillShow() {
  Clear();

  active_index_ = -1;

  int index = 0;
  for (auto& p : profile_.pages) {
    AddRadioItem(0, p.second.title, 0);
    if (main_window_.current_page().id == p.first)
      active_index_ = index;
    ++index;
  }
}

void PageMenuModel::ActivatedAt(int index) {
  auto p = profile_.pages.begin();
  std::advance(p, index);

  auto& page = p->second;

  // check revert page
  bool revert = page.id == main_window_.current_page().id;
  if (revert) {
    std::wstring title = main_window_.current_page().GetTitle();
    std::wstring message = base::StringPrintf(
        L"Вернуться к сохраненному листу %ls?", title.c_str());
    if (dialog_service_.RunMessageBox(message, {},
                                      MessageBoxMode::QuestionYesNo) ==
        MessageBoxResult::No) {
      return;
    }
  }

  // Don't allow to open same page in different windows.
  if (!revert && main_window_manager_.IsPageOpened(page.id)) {
    dialog_service_.RunMessageBox(L"Указанный лист открыт в другом окне.", {},
                                  MessageBoxMode::Info);
    return;
  }

  if (!revert)
    main_window_.SavePage();

  main_window_.OpenPage(page);
}

bool PageMenuModel::IsItemCheckedAt(int index) const {
  return index == active_index_;
}

// WindowMenuModel

void WindowMenuModel::MenuWillShow() {
  Clear();

  active_index_ = -1;

  int index = 0;
  for (auto* opened_view : view_manager_.views()) {
    AddRadioItem(0, opened_view->GetWindowTitle(), 0);
    if (opened_view == main_window_.active_view())
      active_index_ = index;
    ++index;
  }
}

void WindowMenuModel::ActivatedAt(int index) {
  auto& views = view_manager_.views();
  assert(index < static_cast<int>(views.size()));
  auto i = views.begin();
  std::advance(i, index);
  auto& opened_view = **i;
  main_window_.ActivateView(opened_view);
}

bool WindowMenuModel::IsItemCheckedAt(int index) const {
  return index == active_index_;
}

// TrashMenuModel

void TrashMenuModel::MenuWillShow() {
  Clear();

  const auto& trash = profile_.trash;
  for (int i = 0; i < trash.GetWindowCount(); ++i) {
    auto& win = trash.GetWindow(i);
    std::wstring label = L"Восстановить " + win.GetTitle();
    AddItem(0, label);
  }

  empty_ = GetItemCount() == 0;
  if (empty_)
    AddItem(0, L"<Корзина пуста>");
}

void TrashMenuModel::ActivatedAt(int index) {
  Page& trash = profile_.trash;
  assert(index < trash.GetWindowCount());
  main_window_.OpenView(trash.GetWindow(index), true);
  trash.DeleteWindow(index);
}

bool TrashMenuModel::IsEnabledAt(int index) const {
  return !empty_;
}

#if defined(UI_QT)

// StyleMenuModel

StyleMenuModel::StyleMenuModel() : ui::SimpleMenuModel{nullptr} {
  for (const auto& style : QStyleFactory::keys())
    AddRadioItem(0, style.toStdWString(), 0);
}

void StyleMenuModel::ActivatedAt(int index) {
  const auto& style = QString::fromStdWString(GetLabelAt(index));
  QApplication::setStyle(style);
}

bool StyleMenuModel::IsItemCheckedAt(int index) const {
  const auto& style = QString::fromStdWString(GetLabelAt(index));
  return QApplication::style() && QApplication::style()->objectName().compare(
                                      style, Qt::CaseInsensitive) == 0;
}

#endif  // defined(UI_QT)

// MainMenuModel

MainMenuModel::MainMenuModel(const MainMenuContext& context)
    : MainMenuContext{std::move(context)},
      ui::SimpleMenuModel{this},
      display_menu_model_{context},
      table_favourites_{kTableTypes, context},
      table_submenu_{this},
      graph_favourites_{kGraphTypes, context},
      graph_submenu_{this},
      more_submenu_{this},
      page_list_menu_{context},
      page_submenu_{this},
      window_list_menu_{context},
      trash_menu_{context},
      window_submenu_{this},
      settings_submenu_{this},
      help_submenu_{this} {
  Rebuild();
}

void MainMenuModel::Rebuild() {
  AddSubMenu(0, L"Схема", &display_menu_model_);

  table_submenu_.AddItem(ID_TABLE_VIEW, L"Новая таблица");
  table_submenu_.AddItem(ID_SHEET_VIEW, L"Новая п-таблица");
  table_submenu_.AddItem(ID_TIMED_DATA_VIEW, L"Новая таблица значений");
  table_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  table_submenu_.AddItem(ID_OPEN_GROUP_TABLE, L"Таблица группы");
  table_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  table_submenu_.AddInplaceMenu(&table_favourites_);
  AddSubMenu(0, L"Таблица", &table_submenu_);

  graph_submenu_.AddItem(ID_GRAPH_VIEW, L"Новый");
  graph_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  graph_submenu_.AddInplaceMenu(&graph_favourites_);
  AddSubMenu(0, L"График", &graph_submenu_);

  AddSubMenu(0, L"Объект", &context_menu_model_);

  more_submenu_.AddCheckItem(ID_OBJECT_VIEW, L"Объекты");
  more_submenu_.AddCheckItem(ID_EVENT_VIEW, L"События");
  more_submenu_.AddCheckItem(ID_FAVOURITES_VIEW, L"Избранное");
  more_submenu_.AddCheckItem(ID_PORTFOLIO_VIEW, L"Портфолио");
  more_submenu_.AddCheckItem(ID_HARDWARE_VIEW, L"Оборудование");
  more_submenu_.AddCheckItem(ID_EVENT_JOURNAL_VIEW, L"Журнал событий");
  if (admin_) {
    more_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
    more_submenu_.AddCheckItem(ID_NODES_VIEW, L"Узлы");
    more_submenu_.AddCheckItem(ID_TS_FORMATS_VIEW, L"Форматы");
    more_submenu_.AddCheckItem(ID_SIMULATION_ITEMS_VIEW,
                               L"Эмулируемые сигналы");
    more_submenu_.AddCheckItem(ID_USERS_VIEW, L"Пользователи");
    more_submenu_.AddCheckItem(ID_HISTORICAL_DB_VIEW, L"Базы данных");
    more_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
    more_submenu_.AddItem(ID_EXPORT_CONFIGURATION_TO_EXCEL,
                          L"Экспорт конфигурации в Excel...");
    more_submenu_.AddItem(ID_IMPORT_CONFIGURATION_FROM_EXCEL,
                          L"Импорт конфигурации из Excel...");
  }
#if !defined(NDEBUG)
  more_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  more_submenu_.AddItem(ID_LOGIN, L"Подключиться к серверу...");
  more_submenu_.AddItem(ID_LOGOFF, L"Отключиться от сервера");
#endif
  AddSubMenu(0, L"Далее", &more_submenu_);

  justify_index = GetItemCount();

  page_submenu_.AddItem(ID_PAGE_NEW, L"Новый");
  page_submenu_.AddItem(ID_PAGE_DELETE, L"Удалить");
  page_submenu_.AddItem(ID_PAGE_RENAME, L"Переименовать");
  page_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  page_submenu_.AddInplaceMenu(&page_list_menu_);
  AddSubMenu(0, L"Лист", &page_submenu_);

  window_submenu_.AddItem(ID_WINDOW_NEW, L"Новое");
  window_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  window_submenu_.AddItem(ID_VIEW_CHANGE_TITLE, L"Переименовать");
  window_submenu_.AddItem(ID_VIEW_ADD_TO_FAVOURITES, L"В избранное");
  window_submenu_.AddItem(ID_VIEW_CLOSE, L"Закрыть");
#if defined(UI_QT)
  window_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  window_submenu_.AddItem(ID_WINDOW_SPLIT_HORZ, L"Разделить по горизонтали");
  window_submenu_.AddItem(ID_WINDOW_SPLIT_VERT, L"Разделить по вертикали");
#endif
  window_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  window_submenu_.AddInplaceMenu(&window_list_menu_);
  window_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  window_submenu_.AddInplaceMenu(&trash_menu_);
  AddSubMenu(0, L"Окно", &window_submenu_);

  // TODO:
  settings_submenu_.AddCheckItem(0, L"Панель инструментов");
  settings_submenu_.AddCheckItem(ID_VIEW_STATUS_BAR, L"Строка состояния");
  settings_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  settings_submenu_.AddCheckItem(ID_WRITE_CONFIRMATION,
                                 L"Подтверждение управления");
  settings_submenu_.AddCheckItem(ID_SHOW_WRITEOK,
                                 L"Сообщение об успешном управлении");
  settings_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  settings_submenu_.AddCheckItem(ID_SHOW_EVENTS,
                                 L"Показывать события при появлении");
  settings_submenu_.AddCheckItem(ID_HIDE_EVENTS,
                                 L"Скрывать события при квитировании");
  settings_submenu_.AddCheckItem(ID_EVENT_FLASH_WINDOW,
                                 L"Мигание основного окна по событию");
  settings_submenu_.AddCheckItem(ID_EVENT_PLAY_SOUND,
                                 L"Звуковая сигнализация по событию");
  settings_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  settings_submenu_.AddItem(ID_VIEW_PUBLIC_FOLDER, L"Открыть папку схем");
  settings_submenu_.AddCheckItem(ID_MODUS2_MODE,
                                 L"Встроенный визуализатор схем MODUS");

#if defined(UI_QT)
  settings_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  settings_submenu_.AddSubMenu(0, L"Стиль", &style_submenu_);
#endif

  AddSubMenu(0, L"Настройки", &settings_submenu_);

  help_submenu_.AddItem(ID_HELP_MANUAL, L"Документация");
  help_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  if (base::CommandLine::ForCurrentProcess()->HasSwitch("debug")) {
    help_submenu_.AddItem(ID_DUMP_DEBUG_INFO, L"Отладочная информация");
    help_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  }
  help_submenu_.AddItem(ID_APP_ABOUT, L"О программе...");
#if defined(UI_QT)
  help_submenu_.AddItem(ID_ABOUT_QT, L"О Qt...");
#endif
  AddSubMenu(0, L"Справка", &help_submenu_);
}

bool MainMenuModel::IsCommandIdChecked(int command_id) const {
  auto* handler = command_handler_.GetCommandHandler(command_id);
  return handler && handler->IsCommandChecked(command_id);
}

bool MainMenuModel::IsCommandIdEnabled(int command_id) const {
  auto* handler = command_handler_.GetCommandHandler(command_id);
  return handler && handler->IsCommandEnabled(command_id);
}

bool MainMenuModel::GetAcceleratorForCommandId(int command_id,
                                               ui::Accelerator* accelerator) {
  return false;
}

void MainMenuModel::ExecuteCommand(int command_id) {
  if (auto* handler = command_handler_.GetCommandHandler(command_id))
    handler->ExecuteCommand(command_id);
}
