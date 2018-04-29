#include "components/main/views/toolbar_controller.h"

#include "command_handler.h"
#include "components/main/action_manager.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/views/controls/toolbar.h"

ToolbarController::ToolbarController(ActionManager& action_manager,
                                     views::Toolbar& toolbar,
                                     CommandHandler& command_handler)
    : action_manager_{action_manager},
      toolbar_(toolbar),
      command_handler_(command_handler) {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  int image_index = 0;
  for (auto* action_info : action_manager_.actions()) {
    if (action_info->image_id() != 0) {
      toolbar_.SetImage(image_index, rb.GetNamedImage(action_info->image_id()));
      image_indexes_[action_info->image_id()] = image_index;
    }
    ++image_index;
  }

  idle_timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(10), this,
                    &ToolbarController::UpdateCommandStates);
}

void ToolbarController::UpdateCommands() {
  std::vector<views::Toolbar::Item> items;

  std::vector<unsigned> commands;

  const ActionList& actions = action_manager_.actions();
  for (ActionList::const_iterator i = actions.begin(); i != actions.end();
       ++i) {
    const Action& action = **i;
    if (action.always_visible() ||
        command_handler_.GetCommandHandler(action.command_id()))
      commands.push_back(action.command_id());
  }

  auto grouped_commands = GroupCommands(action_manager_, commands);

  for (GroupedActions::iterator i = grouped_commands.begin();
       i != grouped_commands.end(); ++i) {
    CommandCategory category = i->first;
    ActionList& commands = i->second;
    if (commands.empty())
      continue;

    if (i != grouped_commands.begin())
      items.push_back(views::Toolbar::Item(views::Toolbar::SEPARATOR));

    std::vector<views::Toolbar::Item> subitems;

    bool expand_category = CanExpandCommandCategory(category);

    for (ActionList::iterator i = commands.begin(); i != commands.end(); ++i) {
      Action* action = *i;

      CommandHandler* command_handler =
          command_handler_.GetCommandHandler(action->command_id());
      if (!command_handler)
        continue;

      views::Toolbar::Item item(views::Toolbar::BUTTON);
      item.command_id = action->command_id();
      item.text =
          expand_category ? action->GetShortTitle() : action->GetTitle();
      item.image_index_ = GetImageIndex(action->image_id());

      if (!command_handler->IsCommandEnabled(item.command_id))
        item.state |= views::Toolbar::DISABLED;
      if (command_handler->IsCommandChecked(item.command_id))
        item.state |= views::Toolbar::CHECKED;

      if (expand_category)
        items.push_back(item);
      else
        subitems.push_back(item);
    }

    if (!expand_category && !subitems.empty()) {
      views::Toolbar::Item item(views::Toolbar::POPUP);
      item.text = GetCommandCategoryTitle(category);
      item.subitems.swap(subitems);
      items.push_back(item);
    }
  }

  toolbar_.SetItems(items);
}

void ToolbarController::UpdateCommandStates() {
  for (size_t i = 0; i < toolbar_.items().size(); ++i) {
    const views::Toolbar::Item& item = toolbar_.items()[i];
    if (item.type == views::Toolbar::SEPARATOR ||
        item.type == views::Toolbar::POPUP)
      continue;

    unsigned add = 0, remove = 0;

    CommandHandler* handler =
        command_handler_.GetCommandHandler(item.command_id);
    if (!handler) {
      add |= views::Toolbar::DISABLED;
      remove |= views::Toolbar::CHECKED;

    } else {
      if (!handler->IsCommandEnabled(item.command_id))
        add |= views::Toolbar::DISABLED;
      else
        remove |= views::Toolbar::DISABLED;

      if (handler->IsCommandChecked(item.command_id))
        add |= views::Toolbar::CHECKED;
      else
        remove |= views::Toolbar::CHECKED;
    }

    if (add || remove)
      toolbar_.ModifyItemState(i, add, remove);
  }
}

int ToolbarController::GetImageIndex(unsigned command_id) const {
  auto i = image_indexes_.find(command_id);
  return i == image_indexes_.end() ? -1 : i->second;
}
