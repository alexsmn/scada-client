#include "components/main/context_menu_model.h"

#include "components/main/action_manager.h"
#include "components/main/main_window.h"
#include "components/main/opened_view.h"

ContextMenuModel::ContextMenuModel(MainWindow& main_window,
                                   ActionManager& action_manager,
                                   CommandHandler& command_handler)
    : ui::SimpleMenuModel{&command_handler_},
      main_window_{main_window},
      action_manager_{action_manager},
      command_handler_{command_handler} {}

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

  std::vector<unsigned> all_commands;
  for (auto* action : action_manager_.actions()) {
    if (main_window_.active_view() &&
        main_window_.active_view()->commands->GetCommandHandler(
            action->command_id())) {
      all_commands.push_back(action->command_id());
    }
  }

  auto grouped_commands = GroupCommands(action_manager_, all_commands);

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
      auto* submenu = new ui::SimpleMenuModel{&command_handler_};
      submenus_.emplace_back(submenu);
      AddMenuActions(*submenu, commands, main_window_.active_view());

      auto category_title = GetCommandCategoryTitle(category);
      AddSubMenu(0, std::u16string{category_title}, submenu);
      separated = false;
    }
  }
}

void ContextMenuModel::MenuWillShow() {
  Rebuild();
}
