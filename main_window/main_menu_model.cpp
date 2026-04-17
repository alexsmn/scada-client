#include "main_window/main_menu_model.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "base/program_options.h"
#include "base/promise_executor.h"
#include "base/u16format.h"
#include "resources/common_resources.h"
#include "components/debugger/debug_switch.h"
#include "components/sheet/sheet_component.h"
#include "components/table/table_component.h"
#include "components/timed_data/timed_data_component.h"
#include "controller/command_handler.h"
#include "controller/command_registry.h"
#include "controller/window_info.h"
#include "favorites/favourites.h"
#include "filesystem/file_cache.h"
#include "main_window/context_menu_model.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view.h"
#include "main_window/view_manager.h"
#include "profile/profile.h"
#include "profile/window_definition.h"

#if !defined(UI_WT)
#include "graph/graph_component.h"
#include "modus/modus_component.h"
#endif

#include <atlres.h>
#include <ranges>

#if defined(UI_QT)
#include <QApplication>
#include <QStyle>
#include <QStyleFactory>
#endif

namespace {

void AddMenuCommands(aui::SimpleMenuModel& menu,
                     const BasicCommandRegistry<GlobalCommandContext>& commands,
                     MenuGroup menu_group) {
  for (const auto& command : commands.commands()) {
    if (command.menu_group == menu_group) {
      assert(command.command_id != 0);
      assert(!command.title.empty());

      if (command.checked_handler) {
        menu.AddCheckItem(command.command_id, command.title);
      } else {
        menu.AddItem(command.command_id, command.title);
      }
    }
  }
}

std::vector<const WindowInfo*>& GetDisplayMenuWindowInfos() {
  static std::vector<const WindowInfo*> window_infos;
  return window_infos;
}

}  // namespace

void RegisterDisplayMenuWindowInfo(const WindowInfo& window_info) {
  auto& window_infos = GetDisplayMenuWindowInfos();
  if (std::ranges::find(window_infos, &window_info) == window_infos.end()) {
    window_infos.push_back(&window_info);
  }
}

void UnregisterDisplayMenuWindowInfo(const WindowInfo& window_info) {
  auto& window_infos = GetDisplayMenuWindowInfos();
  std::erase(window_infos, &window_info);
}

const WindowInfo* const kTableWindowInfos[] = {
    &kTableWindowInfo, &kSheetWindowInfo, &kTimedDataWindowInfo};

#if !defined(UI_WT)
const WindowInfo* const kGraphWindowInfos[] = {&kGraphWindowInfo};
#endif

// DisplayMenuModel

DisplayMenuModel::DisplayMenuModel(const MainMenuContext& context)
    : MainMenuContext{context}, aui::SimpleMenuModel{nullptr} {}

void DisplayMenuModel::MenuWillShow() {
  Clear();
  items_.clear();

#if !defined(UI_WT)
  AddItems(kModusWindowInfo);
  for (const WindowInfo* window_info : GetDisplayMenuWindowInfos()) {
    AddItems(*window_info);
  }
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
  for (const FileCache::FileEntry& entry :
       file_cache_.GetList(window_info.command_id)) {
    AddItem(0, entry.title);
    items_.emplace_back(&window_info, entry.path);
  }

  if (items_.empty()) {
    AddItem(0, Translate("<No displays>"));
  }
}

// FavouritesMenuModel

FavouritesMenuModel::FavouritesMenuModel(
    std::span<const WindowInfo* const> window_infos,
    const MainMenuContext& context)
    : MainMenuContext{context},
      aui::SimpleMenuModel{nullptr},
      window_infos_{window_infos} {}

void FavouritesMenuModel::MenuWillShow() {
  Clear();
  windows_.clear();

  if (const Page* favourites_folder = favourites_.GetFolder()) {
    for (int i = 0; i != favourites_folder->GetWindowCount(); ++i) {
      const auto& window_def = favourites_folder->GetWindow(i);
      if (const auto* window_info = FindWindowInfoByName(window_def.type)) {
        if (std::ranges::find(window_infos_, window_info) != window_infos_.end()) {
          AddItem(0, window_def.GetTitle(*window_info));
          windows_.push_back(&window_def);
        }
      }
    }
  }

  if (windows_.empty())
    AddItem(0, Translate("<No favourites>"));
}

void FavouritesMenuModel::ActivatedAt(int index) {
  main_window_.OpenView(*windows_[index], true);
}

bool FavouritesMenuModel::IsEnabledAt(int index) const {
  return !windows_.empty();
}

// PageMenuModel

PageMenuModel::PageMenuModel(const MainMenuContext& context)
    : MainMenuContext{context}, aui::SimpleMenuModel{nullptr} {}

void PageMenuModel::MenuWillShow() {
  Clear();

  active_index_ = -1;

  int index = 0;
  for (const auto& [page_id, page] : profile_.pages) {
    AddRadioItem(0, page.title, 0);
    if (main_window_.GetCurrentPage().id == page_id) {
      active_index_ = index;
    }
    ++index;
  }
}

void PageMenuModel::ActivatedAt(int index) {
  auto p = profile_.pages.begin();
  std::advance(p, index);
  OpenPage(p->second);
}

void PageMenuModel::OpenPage(const Page& page) {
  const Page& current_page = main_window_.GetCurrentPage();

  // check revert page
  bool revert = page.id == current_page.id;
  if (!revert) {
    OpenPageHelper(page, false);
    return;
  }

  std::u16string title = current_page.GetTitle();
  std::u16string message =
      u16format(L"Return to saved page {}?", title);
  dialog_service_.RunMessageBox(message, {}, MessageBoxMode::QuestionYesNo)
      .then(BindPromiseExecutor(
          executor_,
          cancelation_.Bind(
              [this, &page](MessageBoxResult message_box_result) {
                if (message_box_result == MessageBoxResult::Yes)
                  OpenPageHelper(page, true);
              })));
}

void PageMenuModel::OpenPageHelper(const Page& page, bool revert) {
  // Don't allow to open same page in different windows.
  if (!revert && main_window_manager_.IsPageOpened(page.id)) {
    dialog_service_.RunMessageBox(Translate("The specified page is open in another window."), {},
                                  MessageBoxMode::Info);
    return;
  }

  if (!revert) {
    main_window_.SaveCurrentPage();
  }

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
  for (const OpenedView* opened_view : view_manager_.views()) {
    AddRadioItem(0, opened_view->GetWindowTitle(), 0);
    if (opened_view == main_window_.GetActiveView()) {
      active_index_ = index;
    }
    ++index;
  }
}

void WindowMenuModel::ActivatedAt(int index) {
  const auto& views = view_manager_.views();
  assert(index < static_cast<int>(views.size()));
  auto i = views.begin();
  std::advance(i, index);
  OpenedView& opened_view = **i;
  main_window_.ActivateView(opened_view);
}

bool WindowMenuModel::IsItemCheckedAt(int index) const {
  return index == active_index_;
}

// TrashMenuModel

void TrashMenuModel::MenuWillShow() {
  Clear();

  const Page& trash = profile_.trash;
  for (int i = 0; i < trash.GetWindowCount(); ++i) {
    const WindowDefinition& window_def = trash.GetWindow(i);
    if (const auto* window_info = FindWindowInfoByName(window_def.type)) {
      std::u16string label =
          Translate("Restore") + u" " + window_def.GetTitle(*window_info);
      AddItem(0, label);
    }
  }

  empty_ = GetItemCount() == 0;
  if (empty_)
    AddItem(0, Translate("<Trash is empty>"));
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
    : MainMenuContext{context},
      aui::SimpleMenuModel{this},
      display_menu_model_{context},
      table_favourites_{std::span{kTableWindowInfos}, context},
      table_submenu_{this},
#if !defined(UI_WT)
      graph_favourites_{std::make_unique<FavouritesMenuModel>(
          std::span{kGraphWindowInfos},
          context)},
#endif
      graph_submenu_{this},
      more_submenu_{this},
      page_list_menu_{context},
      page_submenu_{this},
      window_list_menu_{context},
      trash_menu_{context},
      window_submenu_{this},
#if defined(UI_QT)
      language_submenu_{this},
#endif
      settings_submenu_{this},
      help_submenu_{this} {
  Rebuild();
}

void MainMenuModel::Rebuild() {
  AddSubMenu(0, Translate("Display"), &display_menu_model_);

  table_submenu_.AddItem(ID_TABLE_VIEW, Translate("New Table"));
  table_submenu_.AddItem(ID_SHEET_VIEW, Translate("New Custom Table"));
  table_submenu_.AddItem(ID_TIMED_DATA_VIEW, Translate("New Data Table"));
  table_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  table_submenu_.AddItem(ID_OPEN_GROUP_TABLE, Translate("Group Table"));
  table_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  table_submenu_.AddInplaceMenu(&table_favourites_);
  AddSubMenu(0, Translate("Table"), &table_submenu_);

  graph_submenu_.AddItem(ID_GRAPH_VIEW, Translate("New"));
  graph_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  if (graph_favourites_)
    graph_submenu_.AddInplaceMenu(graph_favourites_.get());
  AddSubMenu(0, Translate("Graph"), &graph_submenu_);

  AddSubMenu(0, Translate("Item"), &context_menu_model_);

  more_submenu_.AddCheckItem(ID_OBJECT_VIEW, Translate("Items"));
  more_submenu_.AddCheckItem(ID_EVENT_VIEW, Translate("Events"));
  more_submenu_.AddCheckItem(ID_FAVOURITES_VIEW, Translate("Favourites"));
  more_submenu_.AddCheckItem(ID_FILE_SYSTEM_VIEW, Translate("Files"));
  more_submenu_.AddCheckItem(ID_PORTFOLIO_VIEW, Translate("Portfolio"));
  more_submenu_.AddCheckItem(ID_HARDWARE_VIEW, Translate("Hardware"));
  more_submenu_.AddCheckItem(ID_EVENT_JOURNAL_VIEW, Translate("Event Journal"));
  if (admin_) {
    more_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
    more_submenu_.AddCheckItem(ID_NODES_VIEW, Translate("Nodes"));
    more_submenu_.AddCheckItem(ID_TS_FORMATS_VIEW, Translate("Formats"));
    more_submenu_.AddCheckItem(ID_SIMULATION_ITEMS_VIEW,
                               Translate("Simulated Signals"));
    more_submenu_.AddCheckItem(ID_USERS_VIEW, Translate("Users"));
    more_submenu_.AddCheckItem(ID_HISTORICAL_DB_VIEW, Translate("Databases"));
    more_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
    more_submenu_.AddItem(ID_EXPORT_CONFIGURATION_TO_EXCEL,
                          Translate("Export Configuration to Excel..."));
    more_submenu_.AddItem(ID_IMPORT_CONFIGURATION_FROM_EXCEL,
                          Translate("Import Configuration from Excel..."));
  }
#if !defined(NDEBUG)
  more_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  more_submenu_.AddItem(ID_LOGIN, Translate("Connect to Server..."));
  more_submenu_.AddItem(ID_LOGOFF, Translate("Disconnect from Server"));
#endif
  AddSubMenu(0, Translate("More"), &more_submenu_);

  justify_index = GetItemCount();

  page_submenu_.AddItem(ID_PAGE_NEW, Translate("New"));
  page_submenu_.AddItem(ID_PAGE_DELETE, Translate("Delete"));
  page_submenu_.AddItem(ID_PAGE_RENAME, Translate("Rename"));
  page_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  page_submenu_.AddInplaceMenu(&page_list_menu_);
  AddSubMenu(0, Translate("Page"), &page_submenu_);

  window_submenu_.AddItem(ID_WINDOW_NEW, Translate("New"));
  window_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  window_submenu_.AddItem(ID_VIEW_CHANGE_TITLE, Translate("Rename"));
  window_submenu_.AddItem(ID_VIEW_ADD_TO_FAVOURITES, Translate("Add to Favourites"));
  window_submenu_.AddItem(ID_VIEW_CLOSE, Translate("Close"));
#if defined(UI_QT)
  window_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  window_submenu_.AddItem(ID_WINDOW_SPLIT_HORZ, Translate("Split Horizontally"));
  window_submenu_.AddItem(ID_WINDOW_SPLIT_VERT, Translate("Split Vertically"));
#endif
  window_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  window_submenu_.AddInplaceMenu(&window_list_menu_);
  window_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  window_submenu_.AddInplaceMenu(&trash_menu_);
  AddSubMenu(0, Translate("Window"), &window_submenu_);

  AddMenuCommands(settings_submenu_, commands_,
                  MenuGroup::MAIN_WINDOW_SETTINGS);

  settings_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  settings_submenu_.AddCheckItem(ID_WRITE_CONFIRMATION,
                                 Translate("Control Confirmation"));
  settings_submenu_.AddCheckItem(ID_SHOW_WRITEOK,
                                 Translate("Control Success Message"));
  settings_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  settings_submenu_.AddCheckItem(ID_SHOW_EVENTS,
                                 Translate("Show Events on Arrival"));
  settings_submenu_.AddCheckItem(ID_HIDE_EVENTS,
                                 Translate("Hide Events on Acknowledge"));
  settings_submenu_.AddCheckItem(ID_EVENT_FLASH_WINDOW,
                                 Translate("Flash Main Window on Event"));
  settings_submenu_.AddCheckItem(ID_EVENT_PLAY_SOUND,
                                 Translate("Sound Alarm on Event"));
  settings_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  settings_submenu_.AddItem(ID_VIEW_PUBLIC_FOLDER, Translate("Open Displays Folder"));

  AddMenuCommands(settings_submenu_, commands_, MenuGroup::DISPLAY_SETTINGS);

#if defined(UI_QT)
  settings_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  language_submenu_.Clear();
  language_submenu_.AddCheckItem(ID_LANGUAGE_ENGLISH, Translate("English"));
  language_submenu_.AddCheckItem(ID_LANGUAGE_RUSSIAN, Translate("Russian"));
  settings_submenu_.AddSubMenu(0, Translate("Language"), &language_submenu_);
  settings_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  settings_submenu_.AddSubMenu(0, Translate("Style"), &style_submenu_);
#endif

  AddSubMenu(0, Translate("Settings"), &settings_submenu_);

  help_submenu_.AddItem(ID_HELP_MANUAL, Translate("Documentation"));
  help_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  if (client::HasOption(kDebugSwitch)) {
    AddMenuCommands(help_submenu_, commands_, MenuGroup::DEBUG);
    help_submenu_.AddItem(ID_DUMP_DEBUG_INFO, Translate("Debug Information"));
    help_submenu_.AddSeparator(aui::NORMAL_SEPARATOR);
  }
  help_submenu_.AddItem(ID_APP_ABOUT, Translate("About..."));
#if defined(UI_QT)
  help_submenu_.AddItem(ID_ABOUT_QT, Translate("About Qt..."));
#endif
  AddSubMenu(0, Translate("Help"), &help_submenu_);
}

bool MainMenuModel::IsCommandIdChecked(int command_id) const {
  const auto* handler = command_handler_.GetCommandHandler(command_id);
  return handler && handler->IsCommandChecked(command_id);
}

bool MainMenuModel::IsCommandIdEnabled(int command_id) const {
  const auto* handler = command_handler_.GetCommandHandler(command_id);
  return handler && handler->IsCommandEnabled(command_id);
}

void MainMenuModel::ExecuteCommand(int command_id) {
  if (auto* handler = command_handler_.GetCommandHandler(command_id)) {
    handler->ExecuteCommand(command_id);
  }
}
