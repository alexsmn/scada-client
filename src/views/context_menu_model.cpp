#include "client/views/context_menu_model.h"

#include "client/components/main/action.h"
#include "client/components/main/opened_view.h"
#include "client/components/main/views/main_window_views.h"

ContextMenuModel::ContextMenuModel(MainWindowViews* main_view, ActionManager& action_manager)
    : ui::SimpleMenuModel(this),
      main_view_(main_view),
      action_manager_(action_manager) {
}

void AddMenuActions(ui::SimpleMenuModel& menu, const ActionList& actions,
                    OpenedView* view) {
  for (ActionList::const_iterator j = actions.begin(); j != actions.end(); ++j) {
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
  const ActionList& actions = action_manager_.actions();
  for (ActionList::const_iterator i = actions.begin(); i != actions.end(); ++i) {
    const Action& action = **i;
    if (main_view_ &&
        main_view_->active_view() &&
        main_view_->active_view()->GetCommandHandler(action.command_id())) {
      commands.push_back(action.command_id());
    }
  }

  auto grouped_commands = GroupCommands(action_manager_, commands);

  bool separated = true;
  for (GroupedActions::iterator i = grouped_commands.begin();
                                i != grouped_commands.end(); ++i) {
    CommandCategory category = i->first;
    const ActionList& commands = i->second;

    if (!separated) {
      AddSeparator(ui::NORMAL_SEPARATOR);
      separated = true;
    }
    
    if (CanExpandCommandCategory(category)) {
      if (main_view_ && !commands.empty()) {
        AddMenuActions(*this, commands, main_view_->active_view());
        separated = false;
      }
        
    } else {
      ui::SimpleMenuModel* submenu = new ui::SimpleMenuModel(this);
      submenus_.push_back(submenu);
      if (main_view_)
        AddMenuActions(*submenu, commands, main_view_->active_view());

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
  if (main_view_)
    main_view_->ExecuteWindowsCommand(command_id);
}
