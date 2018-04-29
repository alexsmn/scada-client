#include "components/main/views/context_menu_model.h"

#include "components/main/action_manager.h"
#include "components/main/opened_view.h"
#include "components/main/views/main_window_views.h"

ContextMenuModel::ContextMenuModel(MainWindowViews& main_window,
                                   ActionManager& action_manager)
    : ui::SimpleMenuModel{this},
      main_window_{main_window},
      action_manager_{action_manager} {}

void AddMenuActions(ui::SimpleMenuModel& menu,
                    const ActionList& actions,
                    OpenedView* view) {
  for (ActionList::const_iterator j = actions.begin(); j != actions.end();
       ++j) {
    const Action& action = **j;
    // Item state is updated on WM_INIMENUPOPUP.
    /*    UINT state = 0;
        if (!view.IsCommandEnabled(action.command_id()))
          state |= MFS_DISABLED;
        if (view.IsCommandChecked(action.command_id()))
          state |= MFS_CHECKED;*/
    menu.AddItem(action.command_id(), action.GetTitle());
  }
}

void ContextMenuModel::Rebuild() {
  Clear();
  submenus_.clear();

  std::vector<unsigned> commands;
  for (auto* action : action_manager_.actions()) {
    if (main_window_.active_view() &&
        main_window_.active_view()->GetCommandHandler(action->command_id())) {
      commands.push_back(action->command_id());
    }
  }

  auto grouped_commands = GroupCommands(action_manager_, commands);

  bool separated = true;
  for (auto& [category, commands] : grouped_commands) {
    if (!separated) {
      AddSeparator(ui::NORMAL_SEPARATOR);
      separated = true;
    }

    if (CanExpandCommandCategory(category)) {
      if (!commands.empty()) {
        AddMenuActions(*this, commands, main_window_.active_view());
        separated = false;
      }

    } else {
      ui::SimpleMenuModel* submenu = new ui::SimpleMenuModel(this);
      submenus_.push_back(submenu);
      AddMenuActions(*submenu, commands, main_window_.active_view());

      const base::char16* category_title = GetCommandCategoryTitle(category);
      AddSubMenu(0, category_title, submenu);
      separated = false;
    }
  }
}

void ContextMenuModel::MenuWillShow() {
  Rebuild();
}

void ContextMenuModel::ExecuteCommand(int command_id) {
  main_window_.ExecuteWindowsCommand(command_id);
}
