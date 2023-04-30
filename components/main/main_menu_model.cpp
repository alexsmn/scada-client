#include "components/main/main_menu_model.h"

#include "base/command_line.h"
#include "base/promise_executor.h"
#include "base/strings/stringprintf.h"
#include "command_handler.h"
#include "common_resources.h"
#include "components/favourites/favourites.h"
#include "components/main/context_menu_model.h"
#include "components/main/main_window.h"
#include "components/main/main_window_manager.h"
#include "components/main/opened_view.h"
#include "components/main/view_manager.h"
#include "components/sheet/sheet_component.h"
#include "components/table/table_component.h"
#include "components/timed_data/timed_data_component.h"
#include "services/dialog_service.h"
#include "services/file_cache.h"
#include "services/profile.h"
#include "window_definition.h"
#include "window_info.h"

#if !defined(UI_WT)
#include "components/graph/graph_component.h"
#include "components/modus/modus_component.h"
#include "components/vidicon_display/vidicon_display_component.h"
#endif

#include <atlres.h>

#if defined(UI_QT)
#include <QApplication>
#include <QStyle>
#include <QStyleFactory>
#endif

const WindowInfo* const kTableWindowInfos[] = {
    &kTableWindowInfo, &kSheetWindowInfo, &kTimedDataWindowInfo};

#if !defined(UI_WT)
const WindowInfo* const kGraphWindowInfos[] = {&kGraphWindowInfo};
#endif

// DisplayMenuModel

DisplayMenuModel::DisplayMenuModel(const MainMenuContext& context)
    : aui::SimpleMenuModel{nullptr}, MainMenuContext{std::move(context)} {}

void DisplayMenuModel::MenuWillShow() {
  Clear();
  items_.clear();

#if !defined(UI_WT)
  AddItems(kModusWindowInfo);
  AddItems(kVidiconDisplayWindowInfo);
#endif
}

void DisplayMenuModel::ActivatedAt(int index) {
  const auto& item = items_[index];
  // find existing display
  if (auto* view = main_window_manager_.FindOpenedViewByFilePath(item.path)) {
    view->Activate();
  } else {
    // add new window
    assert(item.window_info);
    WindowDefinition def(*item.window_info);
    def.path = item.path;
    main_window_.OpenView(def, true);
  }
}

bool DisplayMenuModel::IsEnabledAt(int index) const {
  return !items_.empty();
}

void DisplayMenuModel::AddItems(const WindowInfo& window_info) {
  for (auto& entry : file_cache_.GetList(window_info.command_id)) {
    AddItem(0, entry.title);
    items_.push_back(Item{&window_info, entry.path});
  }

  if (items_.empty())
    AddItem(0, u"<Нет схем>");
}

// FavouritesMenuModel

FavouritesMenuModel::FavouritesMenuModel(
    base::span<const WindowInfo* const> window_infos,
    const MainMenuContext& context)
    : MainMenuContext{std::move(context)},
      aui::SimpleMenuModel{nullptr},
      window_infos_{window_infos} {}

void FavouritesMenuModel::MenuWillShow() {
  Clear();
  windows_.clear();

  if (const Page* favourites_folder = favourites_.GetFolder()) {
    for (int i = 0; i != favourites_folder->GetWindowCount(); ++i) {
      const auto& win = favourites_folder->GetWindow(i);
      if (IsMatchingWindow(win)) {
        AddItem(0, win.GetTitle());
        windows_.push_back(&win);
      }
    }
  }

  if (windows_.empty())
    AddItem(0, u"<Нет избранного>");
}

void FavouritesMenuModel::ActivatedAt(int index) {
  main_window_.OpenView(*windows_[index], true);
}

bool FavouritesMenuModel::IsEnabledAt(int index) const {
  return !windows_.empty();
}

bool FavouritesMenuModel::IsMatchingWindow(
    const WindowDefinition& window) const {
  return std::any_of(window_infos_.begin(), window_infos_.end(),
                     [&window](const WindowInfo* window_info) {
                       return &window.window_info() == window_info;
                     });
}

// PageMenuModel

PageMenuModel::PageMenuModel(const MainMenuContext& context)
    : MainMenuContext{context}, aui::SimpleMenuModel{nullptr} {}

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
  OpenPage(p->second);
}

void PageMenuModel::OpenPage(Page& page) {
  // check revert page
  bool revert = page.id == main_window_.current_page().id;
  if (!revert) {
    OpenPageHelper(page, false);
    return;
  }

  std::u16string title = main_window_.current_page().GetTitle();
  std::u16string message =
      base::StringPrintf(u"Вернуться к сохраненному листу %ls?", title.c_str());
  dialog_service_.RunMessageBox(message, {}, MessageBoxMode::QuestionYesNo)
      .then(BindPromiseExecutor(
          executor_, [this, weak_ptr = weak_ptr_factory_.GetWeakPtr(),
                      &page](MessageBoxResult message_box_result) {
            if (weak_ptr.get() && message_box_result == MessageBoxResult::Yes)
              OpenPageHelper(page, true);
          }));
}

void PageMenuModel::OpenPageHelper(Page& page, bool revert) {
  // Don't allow to open same page in different windows.
  if (!revert && main_window_manager_.IsPageOpened(page.id)) {
    dialog_service_.RunMessageBox(u"Указанный лист открыт в другом окне.", {},
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
    std::u16string label = u"Восстановить " + win.GetTitle();
    AddItem(0, label);
  }

  empty_ = GetItemCount() == 0;
  if (empty_)
    AddItem(0, u"<Корзина пуста>");
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

StyleMenuModel::StyleMenuModel() : aui::SimpleMenuModel{nullptr} {
  for (const auto& style : QStyleFactory::keys())
    AddRadioItem(0, style.toStdU16String(), 0);
}

void StyleMenuModel::ActivatedAt(int index) {
  const auto& style = QString::fromStdU16String(GetLabelAt(index));
  QApplication::setStyle(style);
}

bool StyleMenuModel::IsItemCheckedAt(int index) const {
  const auto& style = QString::fromStdU16String(GetLabelAt(index));
  return QApplication::style() && QApplication::style()->objectName().compare(
                                      style, Qt::CaseInsensitive) == 0;
}

#endif  // defined(UI_QT)

// MainMenuModel

MainMenuModel::MainMenuModel(const MainMenuContext& context)
    : MainMenuContext{std::move(context)},
      aui::SimpleMenuModel{this},
      display_menu_model_{context},
      table_favourites_{base::make_span(kTableWindowInfos), context},
      table_submenu_{this},
#if !defined(UI_WT)
      graph_favourites_{std::make_unique<FavouritesMenuModel>(
          base::make_span(kGraphWindowInfos),
          context)},
#endif
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
  AddSubMenu(0, u"Схема", &display_menu_model_);

  table_submenu_.AddItem(ID_TABLE_VIEW, u"Новая таблица");
  table_submenu_.AddItem(ID_SHEET_VIEW, u"Новая п-таблица");
  table_submenu_.AddItem(ID_TIMED_DATA_VIEW, u"Новая таблица значений");
  table_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  table_submenu_.AddItem(ID_OPEN_GROUP_TABLE, u"Таблица группы");
  table_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  table_submenu_.AddInplaceMenu(&table_favourites_);
  AddSubMenu(0, u"Таблица", &table_submenu_);

  graph_submenu_.AddItem(ID_GRAPH_VIEW, u"Новый");
  graph_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  if (graph_favourites_)
    graph_submenu_.AddInplaceMenu(graph_favourites_.get());
  AddSubMenu(0, u"График", &graph_submenu_);

  AddSubMenu(0, u"Объект", &context_menu_model_);

  more_submenu_.AddCheckItem(ID_OBJECT_VIEW, u"Объекты");
  more_submenu_.AddCheckItem(ID_EVENT_VIEW, u"События");
  more_submenu_.AddCheckItem(ID_FAVOURITES_VIEW, u"Избранное");
  more_submenu_.AddCheckItem(ID_FILE_SYSTEM_VIEW, u"Файлы");
  more_submenu_.AddCheckItem(ID_PORTFOLIO_VIEW, u"Портфолио");
  more_submenu_.AddCheckItem(ID_HARDWARE_VIEW, u"Оборудование");
  more_submenu_.AddCheckItem(ID_EVENT_JOURNAL_VIEW, u"Журнал событий");
  if (admin_) {
    more_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
    more_submenu_.AddCheckItem(ID_NODES_VIEW, u"Узлы");
    more_submenu_.AddCheckItem(ID_TS_FORMATS_VIEW, u"Форматы");
    more_submenu_.AddCheckItem(ID_SIMULATION_ITEMS_VIEW,
                               u"Эмулируемые сигналы");
    more_submenu_.AddCheckItem(ID_USERS_VIEW, u"Пользователи");
    more_submenu_.AddCheckItem(ID_HISTORICAL_DB_VIEW, u"Базы данных");
    more_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
    more_submenu_.AddItem(ID_EXPORT_CONFIGURATION_TO_EXCEL,
                          u"Экспорт конфигурации в Excel...");
    more_submenu_.AddItem(ID_IMPORT_CONFIGURATION_FROM_EXCEL,
                          u"Импорт конфигурации из Excel...");
  }
#if !defined(NDEBUG)
  more_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  more_submenu_.AddItem(ID_LOGIN, u"Подключиться к серверу...");
  more_submenu_.AddItem(ID_LOGOFF, u"Отключиться от сервера");
#endif
  AddSubMenu(0, u"Далее", &more_submenu_);

  justify_index = GetItemCount();

  page_submenu_.AddItem(ID_PAGE_NEW, u"Новый");
  page_submenu_.AddItem(ID_PAGE_DELETE, u"Удалить");
  page_submenu_.AddItem(ID_PAGE_RENAME, u"Переименовать");
  page_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  page_submenu_.AddInplaceMenu(&page_list_menu_);
  AddSubMenu(0, u"Лист", &page_submenu_);

  window_submenu_.AddItem(ID_WINDOW_NEW, u"Новое");
  window_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  window_submenu_.AddItem(ID_VIEW_CHANGE_TITLE, u"Переименовать");
  window_submenu_.AddItem(ID_VIEW_ADD_TO_FAVOURITES, u"В избранное");
  window_submenu_.AddItem(ID_VIEW_CLOSE, u"Закрыть");
#if defined(UI_QT)
  window_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  window_submenu_.AddItem(ID_WINDOW_SPLIT_HORZ, u"Разделить по горизонтали");
  window_submenu_.AddItem(ID_WINDOW_SPLIT_VERT, u"Разделить по вертикали");
#endif
  window_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  window_submenu_.AddInplaceMenu(&window_list_menu_);
  window_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  window_submenu_.AddInplaceMenu(&trash_menu_);
  AddSubMenu(0, u"Окно", &window_submenu_);

  // TODO:
  settings_submenu_.AddCheckItem(0, u"Панель инструментов");
  settings_submenu_.AddCheckItem(ID_VIEW_STATUS_BAR, u"Строка состояния");
  settings_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  settings_submenu_.AddCheckItem(ID_WRITE_CONFIRMATION,
                                 u"Подтверждение управления");
  settings_submenu_.AddCheckItem(ID_SHOW_WRITEOK,
                                 u"Сообщение об успешном управлении");
  settings_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  settings_submenu_.AddCheckItem(ID_SHOW_EVENTS,
                                 u"Показывать события при появлении");
  settings_submenu_.AddCheckItem(ID_HIDE_EVENTS,
                                 u"Скрывать события при квитировании");
  settings_submenu_.AddCheckItem(ID_EVENT_FLASH_WINDOW,
                                 u"Мигание основного окна по событию");
  settings_submenu_.AddCheckItem(ID_EVENT_PLAY_SOUND,
                                 u"Звуковая сигнализация по событию");
  settings_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  settings_submenu_.AddItem(ID_VIEW_PUBLIC_FOLDER, u"Открыть папку схем");
  settings_submenu_.AddCheckItem(ID_MODUS2_MODE,
                                 u"Встроенная отрисовка схем Модус");

#if defined(UI_QT)
  settings_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  settings_submenu_.AddSubMenu(0, u"Стиль", &style_submenu_);
#endif

  AddSubMenu(0, u"Настройки", &settings_submenu_);

  help_submenu_.AddItem(ID_HELP_MANUAL, u"Документация");
  help_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  if (base::CommandLine::ForCurrentProcess()->HasSwitch("debug")) {
    help_submenu_.AddItem(ID_DUMP_DEBUG_INFO, u"Отладочная информация");
    help_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  }
  help_submenu_.AddItem(ID_APP_ABOUT, u"О программе...");
#if defined(UI_QT)
  help_submenu_.AddItem(ID_ABOUT_QT, u"О Qt...");
#endif
  AddSubMenu(0, u"Справка", &help_submenu_);
}

bool MainMenuModel::IsCommandIdChecked(int command_id) const {
  auto* handler = command_handler_.GetCommandHandler(command_id);
  return handler && handler->IsCommandChecked(command_id);
}

bool MainMenuModel::IsCommandIdEnabled(int command_id) const {
  auto* handler = command_handler_.GetCommandHandler(command_id);
  return handler && handler->IsCommandEnabled(command_id);
}

void MainMenuModel::ExecuteCommand(int command_id) {
  if (auto* handler = command_handler_.GetCommandHandler(command_id))
    handler->ExecuteCommand(command_id);
}
