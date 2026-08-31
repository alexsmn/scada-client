#include "settings/settings_catalog.h"

#include "aui/models/menu_model.h"
#include "aui/text_fold.h"
#include "aui/translation.h"
// Toolbar and Status Bar are MFC's standard ids rather than ours, and they live
// apart from the rest for that reason. Macros only -- no link dependency on the
// shell follows from this include.
#include "main_window/standard_command_ids.h"
#include "resources/common_resources.h"

#include <algorithm>
#include <array>

namespace {

using scada::aui::MenuModel;

// Everything about a row that does not depend on the live menu model.
//
// The title is deliberately absent: it comes from the command, so it cannot be
// restated here and drift. What is here is what the menu has no way to say.
struct SettingDescriptor {
  unsigned command_id;
  std::string_view id;
  SettingCategory category;
  SettingScope scope;
  SettingControl control;
  // English source, translated at build time the same way every other client
  // string is.
  const char* description;
};

// The inventory, in screen order.
//
// Fourteen rows over five categories, which is the Qt half of the twenty rows
// `settings-rows.html` counts: six of that sheet's rows are web-only (the two
// layout sliders, the rail and Explorer switches, the plant name and the
// configuration snapshot) and are absent here rather than drawn disabled.
//
// Every entry names a command the shell registers today. Adding one without a
// command is how an invented preference would get in, which is the failure the
// screen's own authoring rules exist to prevent -- so the join is checked in
// both directions by `UndescribedSettingsCommands` and by
// `settings_catalog_unittest.cpp`.
constexpr std::array kDescriptors = {
    SettingDescriptor{
        .command_id = ID_SETTINGS_LANGUAGE,
        .id = "language",
        .category = SettingCategory::kAppearance,
        .scope = SettingScope::kClient,
        .control = SettingControl::kChoice,
        .description =
            "Language of the operator interface. Stored on this machine, so it "
            "does not follow the account to another workstation — or to the "
            "web "
            "client.",
    },
    SettingDescriptor{
        .command_id = ID_SETTINGS_APPEARANCE,
        .id = "colour-scheme",
        .category = SettingCategory::kAppearance,
        .scope = SettingScope::kClient,
        .control = SettingControl::kChoice,
        .description =
            "Follow system, Dark, Light or High contrast. The choice applies "
            "live and in full. Alarm severity, data quality and switchgear "
            "state keep their fixed values whichever scheme is picked: those "
            "are safety signals, not styling.",
    },
    SettingDescriptor{
        .command_id = ID_SETTINGS_STYLE,
        .id = "widget-style",
        .category = SettingCategory::kAppearance,
        .scope = SettingScope::kClient,
        .control = SettingControl::kChoice,
        .description = "The platform style controls are drawn with. The colour "
                       "scheme layers "
                       "over it rather than replacing it, which is why the two "
                       "are separate "
                       "settings.",
    },
    SettingDescriptor{
        .command_id = ID_EVENT_PLAY_SOUND,
        .id = "sound-on-events",
        .category = SettingCategory::kEventsAlarms,
        .scope = SettingScope::kProfile,
        .control = SettingControl::kToggle,
        .description =
            "Play the annunciator tone while an event stands unacknowledged. "
            "One "
            "key on both clients, so a choice made in either reaches the "
            "other. It "
            "silences the tone only: the banner and the severity colours are "
            "ISA-18.2 signals and stay unconditional.",
    },
    SettingDescriptor{
        .command_id = ID_SHOW_EVENTS,
        .id = "show-events",
        .category = SettingCategory::kEventsAlarms,
        .scope = SettingScope::kProfile,
        .control = SettingControl::kToggle,
        .description = "Raise the event view when an event arrives.",
    },
    SettingDescriptor{
        .command_id = ID_HIDE_EVENTS,
        .id = "hide-events",
        .category = SettingCategory::kEventsAlarms,
        .scope = SettingScope::kProfile,
        .control = SettingControl::kToggle,
        .description =
            "Drop an event from the view once it has been acknowledged.",
    },
    SettingDescriptor{
        .command_id = ID_EVENT_FLASH_WINDOW,
        .id = "flash-window",
        .category = SettingCategory::kEventsAlarms,
        .scope = SettingScope::kProfile,
        .control = SettingControl::kToggle,
        .description = "Flash the window in the taskbar when an event arrives.",
    },
    SettingDescriptor{
        .command_id = ID_WRITE_CONFIRMATION,
        .id = "control-confirmation",
        .category = SettingCategory::kControl,
        .scope = SettingScope::kProfile,
        .control = SettingControl::kToggle,
        .description =
            "Confirm before sending a control command. The web client confirms "
            "every control unconditionally, so it publishes no switch rather "
            "than "
            "one that is always on.",
    },
    SettingDescriptor{
        .command_id = ID_SHOW_WRITEOK,
        .id = "control-success",
        .category = SettingCategory::kControl,
        .scope = SettingScope::kProfile,
        .control = SettingControl::kToggle,
        .description =
            "Report a successful control command back to the operator.",
    },
    SettingDescriptor{
        .command_id = ID_VIEW_TOOLBAR,
        .id = "toolbar",
        .category = SettingCategory::kWorkspace,
        .scope = SettingScope::kWindow,
        .control = SettingControl::kToggle,
        .description =
            "Show the grip command toolbar in this window. Off by default — "
            "the context bar carries the same commands — and per main window, "
            "not per account, so a second window keeps its own answer.",
    },
    SettingDescriptor{
        .command_id = ID_VIEW_STATUS_BAR,
        .id = "status-bar",
        .category = SettingCategory::kWorkspace,
        .scope = SettingScope::kWindow,
        .control = SettingControl::kToggle,
        .description = "Show the status strip in this window.",
    },
    SettingDescriptor{
        .command_id = ID_MODUS_TOPOLOGY,
        .id = "modus-topology",
        .category = SettingCategory::kDisplays,
        .scope = SettingScope::kProfile,
        .control = SettingControl::kToggle,
        .description = "Draw the Modus topology layer over the display.",
    },
    SettingDescriptor{
        .command_id = ID_MODUS_RUNTIME_RENDERER,
        .id = "modus-renderer",
        .category = SettingCategory::kDisplays,
        .scope = SettingScope::kProfile,
        .control = SettingControl::kToggle,
        .description =
            "Render Modus displays with the runtime renderer instead of the "
            "ActiveX control.",
    },
    SettingDescriptor{
        .command_id = ID_VIEW_PUBLIC_FOLDER,
        .id = "open-displays-folder",
        .category = SettingCategory::kDisplays,
        .scope = SettingScope::kAction,
        .control = SettingControl::kAction,
        .description =
            "Open the folder displays are stored in. An action, not a "
            "preference "
            "— which is why the preferences dialog dropped it, since a command "
            "drawn as a checkbox would misreport it. A full-window surface has "
            "room to keep it beside the two switches it belongs with.",
    },
};
constexpr std::array kCategoryOrder = {
    SettingCategory::kAppearance, SettingCategory::kEventsAlarms,
    SettingCategory::kControl,    SettingCategory::kWorkspace,
    SettingCategory::kDisplays,
};

constexpr std::array kStorageScopes = {
    SettingScope::kClient,
    SettingScope::kProfile,
    SettingScope::kWindow,
};

// Locates `command_id` in the settings menu, or a null model when the shell
// does not publish it. Search rather than a cached index: the menu is rebuilt
// whenever a module's contribution changes, and an index taken once would
// outlive the item it named.
struct MenuItemRef {
  MenuModel* model = nullptr;
  int index = 0;
};

MenuItemRef FindMenuItem(MenuModel& settings_menu, unsigned command_id) {
  MenuModel* model = &settings_menu;
  int index = 0;
  if (!MenuModel::GetModelAndIndexForCommandId(static_cast<int>(command_id),
                                               &model, &index)) {
    return {};
  }
  return {model, index};
}

// Walks the settings menu the way `SettingsDialog::BuildSection` did, calling
// `visit` for every visible item that carries a command. In-place menus
// contribute their items to the enclosing section rather than a group of their
// own, which is what "in-place" means in the menu too.
template <class Visit>
void ForEachSettingsCommand(MenuModel& model, const Visit& visit) {
  for (int index = 0; index < model.GetItemCount(); ++index) {
    if (!model.IsVisibleAt(index))
      continue;

    switch (model.GetTypeAt(index)) {
      case MenuModel::TYPE_SEPARATOR:
        break;

      case MenuModel::TYPE_CHECK:
      case MenuModel::TYPE_RADIO:
      case MenuModel::TYPE_COMMAND:
      case MenuModel::TYPE_BUTTON_ITEM:
      case MenuModel::TYPE_SUBMENU:
        visit(model, index);
        break;

      case MenuModel::TYPE_INPLACE_MENU:
        if (MenuModel* inplace = model.GetSubmenuModelAt(index)) {
          inplace->MenuWillShow();
          ForEachSettingsCommand(*inplace, visit);
        }
        break;
    }
  }
}

}  // namespace

std::span<const SettingCategory> SettingCategoryOrder() {
  return kCategoryOrder;
}

std::span<const SettingScope> SettingStorageScopes() {
  return kStorageScopes;
}

std::u16string SettingCategoryLabel(SettingCategory category) {
  switch (category) {
    case SettingCategory::kAppearance:
      return Translate("Appearance");
    case SettingCategory::kEventsAlarms:
      return Translate("Events & alarms");
    case SettingCategory::kControl:
      return Translate("Control");
    case SettingCategory::kWorkspace:
      return Translate("Workspace");
    case SettingCategory::kDisplays:
      return Translate("Displays");
  }
  return {};
}

std::u16string SettingScopeLabel(SettingScope scope) {
  switch (scope) {
    // Named from the operator's side of the question -- "will this follow me?"
    // -- rather than after the store that answers it.
    case SettingScope::kClient:
      return Translate("This client");
    case SettingScope::kProfile:
      return Translate("Profile");
    case SettingScope::kWindow:
      return Translate("This window");
    case SettingScope::kAction:
      return Translate("Action");
  }
  return {};
}

std::span<const unsigned> SettingsCatalogCommandIds() {
  // Built once from the descriptor table rather than declared beside it, so the
  // two cannot list different commands.
  static const std::vector<unsigned> ids = [] {
    std::vector<unsigned> result;
    result.reserve(kDescriptors.size());
    for (const SettingDescriptor& descriptor : kDescriptors)
      result.push_back(descriptor.command_id);
    return result;
  }();
  return ids;
}

std::vector<SettingRow> BuildSettingsCatalog(MenuModel& settings_menu) {
  std::vector<SettingRow> rows;
  rows.reserve(kDescriptors.size());

  for (const SettingDescriptor& descriptor : kDescriptors) {
    const MenuItemRef item = FindMenuItem(settings_menu, descriptor.command_id);
    // Not published by this build, or hidden from this session: no row. A door
    // to a setting that is not there is worse than no door.
    if (!item.model || !item.model->IsVisibleAt(item.index))
      continue;

    rows.push_back(SettingRow{
        .id = descriptor.id,
        .command_id = descriptor.command_id,
        .category = descriptor.category,
        .scope = descriptor.scope,
        .control = descriptor.control,
        .title = item.model->GetLabelAt(item.index),
        .description = Translate(descriptor.description),
        .model = item.model,
        .index = item.index,
    });
  }

  return rows;
}

std::vector<unsigned> UndescribedSettingsCommands(MenuModel& settings_menu) {
  const std::span<const unsigned> described = SettingsCatalogCommandIds();

  std::vector<unsigned> undescribed;
  ForEachSettingsCommand(settings_menu, [&](MenuModel& model, int index) {
    const int command_id = model.GetCommandIdAt(index);
    // A submenu with no id of its own is the menu's own structure rather than a
    // preference; the three choice submenus carry ids precisely so they are not
    // in that category.
    if (command_id <= 0)
      return;
    const auto id = static_cast<unsigned>(command_id);
    if (std::ranges::find(described, id) != described.end())
      return;
    if (std::ranges::find(undescribed, id) != undescribed.end())
      return;
    undescribed.push_back(id);
  });
  return undescribed;
}

bool SettingRowMatchesQuery(const SettingRow& row, std::u16string_view query) {
  const std::u16string folded_query = FoldForSearch(query);
  // The whole searchable text of the row, folded once: the category name is in
  // it because "displays" is how an operator looks for a display setting whose
  // title never says the word.
  const std::u16string haystack =
      FoldForSearch(row.title + u' ' + row.description + u' ' +
                    SettingCategoryLabel(row.category));

  size_t position = 0;
  while (position < folded_query.size()) {
    const size_t start = folded_query.find_first_not_of(u" \t\n", position);
    if (start == std::u16string::npos)
      break;
    size_t end = folded_query.find_first_of(u" \t\n", start);
    if (end == std::u16string::npos)
      end = folded_query.size();
    if (haystack.find(folded_query.substr(start, end - start)) ==
        std::u16string::npos) {
      return false;
    }
    position = end;
  }
  return true;
}

std::vector<SettingRow> FilterSettingRows(std::span<const SettingRow> rows,
                                          std::u16string_view query,
                                          std::optional<SettingScope> scope) {
  std::vector<SettingRow> result;
  for (const SettingRow& row : rows) {
    if (scope && row.scope != *scope)
      continue;
    if (!SettingRowMatchesQuery(row, query))
      continue;
    result.push_back(row);
  }
  return result;
}

std::vector<SettingCategoryGroup> GroupSettingRows(
    std::span<const SettingRow> rows) {
  std::vector<SettingCategoryGroup> groups;
  for (SettingCategory category : kCategoryOrder) {
    SettingCategoryGroup group{.category = category,
                               .label = SettingCategoryLabel(category)};
    for (const SettingRow& row : rows) {
      if (row.category == category)
        group.rows.push_back(row);
    }
    if (!group.rows.empty())
      groups.push_back(std::move(group));
  }
  return groups;
}

std::vector<SettingScope> VisibleSettingScopes(
    std::span<const SettingRow> rows) {
  std::vector<SettingScope> scopes;
  for (SettingScope scope : kStorageScopes) {
    const bool used = std::ranges::any_of(
        rows, [scope](const SettingRow& row) { return row.scope == scope; });
    if (used)
      scopes.push_back(scope);
  }
  return scopes;
}

SettingRowCounts CountSettingRows(std::span<const SettingRow> rows) {
  SettingRowCounts counts;
  for (const SettingRow& row : rows) {
    if (row.scope == SettingScope::kAction)
      ++counts.actions;
    else
      ++counts.settings;
  }
  return counts;
}
