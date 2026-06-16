#include "main_window/context_menu_model.h"

#include "controller/action_manager.h"
#include "controller/command_manager.h"
#include "main_window/main_window.h"
#include "main_window/opened_view/opened_view.h"

namespace {

constexpr CommandContextId kContextMenuContexts[] = {
    CommandContextId::Global,
    CommandContextId::Selection,
    CommandContextId::OpenedView,
    CommandContextId::Controller,
};

}  // namespace

ContextMenuModel::ContextMenuModel(MainWindowInterface& main_window,
                                   CommandManager& command_manager,
                                   CommandHandler& command_handler)
    : aui::SimpleMenuModel{&command_handler_},
      main_window_{main_window},
      command_manager_{command_manager},
      command_handler_{command_handler} {}

void AddMenuActions(aui::SimpleMenuModel& menu,
                    const CommandDescriptorList& commands,
                    OpenedView* view) {
  for (const auto* command : commands) {
    // Item state is updated on WM_INIMENUPOPUP.
    /*    UINT state = 0;
        if (!view.IsCommandEnabled(action.command_id()))
          state |= MFS_DISABLED;
        if (view.IsCommandChecked(action.command_id()))
          state |= MFS_CHECKED;*/
    if (command->checkable()) {
      menu.AddCheckItem(command->command_id, command->GetTitle());
    } else {
      menu.AddItem(command->command_id, command->GetTitle());
    }
  }
}

void ContextMenuModel::Rebuild() {
  Clear();
  submenus_.clear();

  auto* active_view = static_cast<OpenedView*>(main_window_.GetActiveView());
  if (!active_view) {
    return;
  }

  std::vector<unsigned> all_commands;
  for (const CommandDescriptor* command : command_manager_.commands()) {
    auto* handler = command_manager_.ResolveHandler(command->command_id,
                                                    kContextMenuContexts);
    if (!handler) {
      handler = active_view->commands->GetCommandHandler(command->command_id);
    }

    if (command->show_in_context_menu && handler) {
      all_commands.push_back(command->command_id);
    }
  }

  auto grouped_commands = GroupCommands(command_manager_, all_commands);

  bool separated = true;
  for (const auto& [category, commands] : grouped_commands) {
    if (!separated) {
      AddSeparator(aui::NORMAL_SEPARATOR);
      separated = true;
    }

    if (CanExpandCommandCategory(category)) {
      if (!commands.empty()) {
        AddMenuActions(*this, commands, active_view);
        separated = false;
      }

    } else {
      auto* submenu = new aui::SimpleMenuModel{&command_handler_};
      submenus_.emplace_back(submenu);
      AddMenuActions(*submenu, commands, active_view);

      auto category_title = GetCommandCategoryTitle(category);
      AddSubMenu(0, std::u16string{category_title}, submenu);
      separated = false;
    }
  }
}

void ContextMenuModel::MenuWillShow() {
  Rebuild();
}
