#include "main_window/main_menu/main_menu_model.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "base/check.h"
#include "base/program_options.h"
#include "base/u16format.h"
#include "controller/command_handler.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/main_menu_window_type_registry.h"
#include "controller/window_info.h"
#include "favorites/favourites.h"
#include "filesystem/file_cache.h"
#include "main_window/context_menu_model.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view/opened_view.h"
#include "main_window/standard_command_ids.h"
#include "main_window/view_manager.h"
#include "modules/debugger/debug_switch.h"
#include "net/net_executor_adapter.h"
#include "profile/profile.h"
#include "profile/window_definition.h"
#include "resources/common_resources.h"

#include <ranges>
#include <string>

#if defined(UI_QT)
#include <QApplication>
#include <QStyle>
#include <QStyleFactory>
#endif

namespace {

void AddMenuCommands(scada::aui::SimpleMenuModel& menu,
                     const BasicCommandRegistry<GlobalCommandContext>& commands,
                     MenuGroup menu_group) {
  for (const auto& command : commands.commands()) {
    if (command.menu_group == menu_group) {
      scada::base::Check(command.command_id != 0);
      scada::base::Check(!command.title.empty());

      if (command.checked_handler) {
        menu.AddCheckItem(command.command_id, command.title);
      } else {
        menu.AddItem(command.command_id, command.title);
      }
    }
  }
}

void AddMenuContributions(
    scada::aui::SimpleMenuModel& menu,
    const UiCommandRegistry& ui_command_registry,
    const BasicCommandRegistry<GlobalCommandContext>& commands,
    MainMenuId menu_id,
    bool admin) {
  for (const auto& contribution :
       ui_command_registry.GetMenuContributions(menu_id)) {
    if (contribution.admin_only && !admin) {
      continue;
    }
    if (contribution.debug_only && !client::HasOption(kDebugSwitch)) {
      continue;
    }

    if (contribution.separator_before) {
      menu.AddSeparator(scada::aui::NORMAL_SEPARATOR);
    }

    auto title = contribution.title;
    if (title.empty()) {
      if (const auto* command =
              ui_command_registry.command_manager().FindCommand(
                  contribution.command_id)) {
        title = command->GetTitle();
      }
    }
    if (title.empty()) {
      if (const auto* command = commands.FindCommand(contribution.command_id)) {
        title = command->title;
      }
    }
    scada::base::Check(!title.empty());

    if (contribution.checkable) {
      menu.AddCheckItem(contribution.command_id, title);
    } else {
      menu.AddItem(contribution.command_id, title);
    }
  }
}

}  // namespace

// DisplayMenuModel

DisplayMenuModel::DisplayMenuModel(const MainMenuContext& context)
    : MainMenuContext{context}, scada::aui::SimpleMenuModel{nullptr} {}

void DisplayMenuModel::MenuWillShow() {
  Clear();
  items_.clear();

  for (const std::string& window_type : GetDisplayMenuWindowTypes()) {
    if (const auto* window_info = FindWindowInfoByName(window_type)) {
      AddItems(*window_info);
    }
  }
}

void DisplayMenuModel::ActivatedAt(int index) {
  const auto& item = items_[index];
  // find existing display
  if (auto* view = main_window_manager_.FindOpenedViewByFilePath(item.path)) {
    view->Activate();
  } else {
    // add new window
    scada::base::Check(item.window_info);
    WindowDefinition def(*item.window_info);
    def.path = item.path;
    CoSpawn(executor_, [this, def = std::move(def)]() -> Awaitable<void> {
      co_await main_window_.OpenView(def, true);
    });
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

FavouritesMenuModel::FavouritesMenuModel(MainMenuId menu_id,
                                         const MainMenuContext& context)
    : MainMenuContext{context},
      scada::aui::SimpleMenuModel{nullptr},
      menu_id_{menu_id} {}

void FavouritesMenuModel::MenuWillShow() {
  Clear();
  windows_.clear();

  if (const Page* favourites_folder = favourites_.GetFolder()) {
    for (int i = 0; i != favourites_folder->GetWindowCount(); ++i) {
      const auto& window_def = favourites_folder->GetWindow(i);
      if (const auto* window_info = FindWindowInfoByName(window_def.type)) {
        const auto& window_types = GetFavouritesMenuWindowTypes(menu_id_);
        if (std::ranges::find(window_types, window_def.type) !=
            window_types.end()) {
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
  CoSpawn(executor_, [this, window = *windows_[index]]() -> Awaitable<void> {
    co_await main_window_.OpenView(window, true);
  });
}

bool FavouritesMenuModel::IsEnabledAt(int index) const {
  return !windows_.empty();
}

// PageMenuModel

PageMenuModel::PageMenuModel(const MainMenuContext& context)
    : MainMenuContext{context},
      scada::aui::SimpleMenuModel{nullptr},
      page_switcher_{PageSwitcherContext{
          .executor_ = context.executor_,
          .profile_ = context.profile_,
          .main_window_ = context.main_window_,
          .main_window_manager_ = context.main_window_manager_,
          .dialog_service_ = context.dialog_service_}} {}

void PageMenuModel::MenuWillShow() {
  Clear();

  active_index_ = -1;
  entries_ = page_switcher_.ListPages();

  for (int index = 0; index < static_cast<int>(entries_.size()); ++index) {
    AddRadioItem(0, entries_[index].title, 0);
    if (entries_[index].current)
      active_index_ = index;
  }
}

void PageMenuModel::ActivatedAt(int index) {
  if (index < 0 || index >= static_cast<int>(entries_.size()))
    return;
  page_switcher_.ActivatePage(entries_[index].page_id);
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
  scada::base::Check(index < static_cast<int>(views.size()));
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
  scada::base::Check(index < trash.GetWindowCount());
  auto window = trash.GetWindow(index);
  trash.DeleteWindow(index);
  CoSpawn(executor_, [this, window = std::move(window)]() -> Awaitable<void> {
    co_await main_window_.OpenView(window, true);
  });
}

bool TrashMenuModel::IsEnabledAt(int index) const {
  return !empty_;
}

#if defined(UI_QT)

// StyleMenuModel

StyleMenuModel::StyleMenuModel() : scada::aui::SimpleMenuModel{nullptr} {
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

// AppearanceMenuModel

AppearanceMenuModel::AppearanceMenuModel(const MainMenuContext& context)
    : MainMenuContext{context}, scada::aui::SimpleMenuModel{nullptr} {
  // One radio group: the platform look and the token themes are alternatives,
  // and "Classic" is the shipped default rather than an escape hatch, so it
  // leads. The rule below it separates "no theme" from the themes without
  // implying a second group.
  AddRadioItem(0, Translate("Classic"), 0);
  rows_.push_back({.kind = Row::Kind::kClassic});

  AddSeparator(scada::aui::NORMAL_SEPARATOR);
  rows_.push_back({.kind = Row::Kind::kSeparator});

  // `kSystem` first, and named for what it does: it is the default appearance
  // once the reshell is on, because the client is a native desktop application
  // and follows the host light/dark preference (docs/client/ux/principles.md
  // §9). High contrast is only ever reached by choosing it — Qt exposes no
  // portable OS high-contrast signal.
  const std::pair<std::u16string, scada::aui::Theme> kThemes[] = {
      {Translate("Follow system"), scada::aui::Theme::kSystem},
      {Translate("Dark"), scada::aui::Theme::kDark},
      {Translate("Light"), scada::aui::Theme::kLight},
      {Translate("High contrast"), scada::aui::Theme::kHighContrast},
  };
  for (const auto& [label, theme] : kThemes) {
    AddRadioItem(0, label, 0);
    rows_.push_back({.kind = Row::Kind::kTheme, .theme = theme});
  }
}

void AppearanceMenuModel::ActivatedAt(int index) {
  scada::base::Check(index >= 0 && index < static_cast<int>(rows_.size()));
  const Row& row = rows_[index];
  if (row.kind == Row::Kind::kSeparator) {
    // Not selectable: BuildMenu turns separators into QMenu separators, which
    // carry no action to trigger.
    return;
  }

  const bool was_installed = scada::aui::IsThemeInstalled();
  if (row.kind == Row::Kind::kClassic) {
    scada::aui::ClearTheme();
  } else {
    // Scope deliberately left at the default: `Ux/StyleSheet` is an expert
    // escape hatch read once at startup, not something a menu row toggles.
    scada::aui::ApplyTheme(row.theme);
  }

  if (scada::aui::IsThemeInstalled() != was_installed) {
    NotifyShellFollowsOnRestart();
  }
}

void AppearanceMenuModel::NotifyShellFollowsOnRestart() {
  CoSpawn(executor_, [this]() -> Awaitable<void> {
    co_await dialog_service_.RunMessageBox(
        Translate("The colours have changed. The workbench layout — activity "
                  "bar, context bar and Inspector — follows when the client is "
                  "restarted."),
        Translate("Colour scheme"), MessageBoxMode::Info);
  });
}

bool AppearanceMenuModel::IsItemCheckedAt(int index) const {
  scada::base::Check(index >= 0 && index < static_cast<int>(rows_.size()));
  const Row& row = rows_[index];
  const bool installed = scada::aui::IsThemeInstalled();
  switch (row.kind) {
    case Row::Kind::kSeparator:
      return false;
    case Row::Kind::kClassic:
      return !installed;
    case Row::Kind::kTheme:
      // Compare against the theme as chosen, not as resolved: while following
      // the system, "Follow system" is the checked row — checking "Dark"
      // because the desktop happens to be dark would misreport the choice.
      return installed && scada::aui::ActiveTheme() == row.theme;
  }
  return false;
}

#endif  // defined(UI_QT)

// MainMenuModel

MainMenuModel::MainMenuModel(const MainMenuContext& context)
    : MainMenuContext{context},
      scada::aui::SimpleMenuModel{this},
      display_menu_model_{context},
      table_favourites_{MainMenuId::Table, context},
      table_submenu_{this},
      graph_favourites_{
          std::make_unique<FavouritesMenuModel>(MainMenuId::Graph, context)},
      graph_submenu_{this},
      more_submenu_{this},
      page_list_menu_{context},
      page_submenu_{this},
      window_list_menu_{context},
      trash_menu_{context},
      window_submenu_{this},
#if defined(UI_QT)
      appearance_submenu_{context},
      language_submenu_{this},
      settings_menu_{this},
#endif
      settings_submenu_{this},
      help_submenu_{this} {
  Rebuild();
}

void MainMenuModel::Rebuild() {
  AddSubMenu(0, Translate("Display"), &display_menu_model_);

  AddMenuContributions(table_submenu_, ui_command_registry_, commands_,
                       MainMenuId::Table, admin_);
  table_submenu_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  table_submenu_.AddInplaceMenu(&table_favourites_);
  AddSubMenu(0, Translate("Table"), &table_submenu_);

  AddMenuContributions(graph_submenu_, ui_command_registry_, commands_,
                       MainMenuId::Graph, admin_);
  graph_submenu_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  if (graph_favourites_)
    graph_submenu_.AddInplaceMenu(graph_favourites_.get());
  AddSubMenu(0, Translate("Graph"), &graph_submenu_);

  AddSubMenu(0, Translate("Item"), &context_menu_model_);

  AddMenuContributions(more_submenu_, ui_command_registry_, commands_,
                       MainMenuId::More, admin_);
  AddSubMenu(0, Translate("More"), &more_submenu_);

  justify_index = GetItemCount();

  AddMenuContributions(page_submenu_, ui_command_registry_, commands_,
                       MainMenuId::Page, admin_);
  page_submenu_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  page_submenu_.AddInplaceMenu(&page_list_menu_);
  AddSubMenu(0, Translate("Page"), &page_submenu_);

  AddMenuContributions(window_submenu_, ui_command_registry_, commands_,
                       MainMenuId::Window, admin_);
  window_submenu_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  window_submenu_.AddInplaceMenu(&window_list_menu_);
  window_submenu_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  window_submenu_.AddInplaceMenu(&trash_menu_);
  AddSubMenu(0, Translate("Window"), &window_submenu_);

  AddMenuCommands(settings_submenu_, commands_,
                  MenuGroup::MAIN_WINDOW_SETTINGS);
  AddMenuContributions(settings_submenu_, ui_command_registry_, commands_,
                       MainMenuId::Settings, admin_);

  AddMenuCommands(settings_submenu_, commands_, MenuGroup::DISPLAY_SETTINGS);

#if defined(UI_QT)
  settings_submenu_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  language_submenu_.Clear();
  AddMenuContributions(language_submenu_, ui_command_registry_, commands_,
                       MainMenuId::Language, admin_);
  // The three choice submenus carry command ids, which nothing dispatches -- a
  // submenu is opened, never executed. They exist so `settings_catalog.cpp` can
  // name a choice row by command id, the way it names every other row, rather
  // than by its position among the submenus or by a translated label.
  settings_submenu_.AddSubMenu(ID_SETTINGS_LANGUAGE, Translate("Language"),
                               &language_submenu_);
  settings_submenu_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  settings_submenu_.AddSubMenu(ID_SETTINGS_STYLE, Translate("Style"),
                               &style_submenu_);
  // Next to Style, not inside it: the appearance themes layer over the widget
  // style rather than replacing it (see AppearanceMenuModel).
  settings_submenu_.AddSubMenu(
      ID_SETTINGS_APPEARANCE, Translate("Colour scheme"), &appearance_submenu_);

  // The Qt shell shows the toggles on the full-window Settings surface, so its
  // Settings menu holds one item that opens it. `settings_submenu_` is still
  // fully populated above — it is the description `settings_catalog.cpp` joins
  // its rows to — it is just not what gets mounted, so the toggles reach the
  // operator from exactly one place. A top-level menu-bar entry has to be a
  // menu (CreateMenuBar requires a submenu model for every one), which is why
  // this is a one-item menu rather than a bare item.
  settings_menu_.Clear();
  settings_menu_.AddItem(ID_SETTINGS, Translate("Settings..."));
  AddSubMenu(0, Translate("Settings"), &settings_menu_);
#else
  // Without the Qt UI config there is no Settings surface, so the toggles stay
  // in the menu.
  AddSubMenu(0, Translate("Settings"), &settings_submenu_);
#endif

  AddMenuContributions(help_submenu_, ui_command_registry_, commands_,
                       MainMenuId::Help, admin_);
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
