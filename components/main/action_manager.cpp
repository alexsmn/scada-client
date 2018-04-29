#include "components/main/action_manager.h"

#include "base/strings/sys_string_conversions.h"
#include "common_resources.h"

GroupedActions GroupCommands(ActionManager& action_manager,
                             const std::vector<unsigned>& commands) {
  GroupedActions grouped_commands;
  for (unsigned command_id : commands) {
    if (auto* action = action_manager.FindAction(command_id))
      grouped_commands[action->category_].push_back(action);
  }
  return grouped_commands;
}

const base::char16* GetCommandCategoryTitle(CommandCategory category) {
  const base::char16* titles[] = {L"Новый",       // CATEGORY_NEW
                                  L"Открыть",     // CATEGORY_OPEN
                                  L"Объект",      // CATEGORY_ITEM
                                  L"Устройство",  // CATEGORY_DEVICE
                                  L"Опции",       // CATEGORY_SETUP
                                  L"Разное",      // CATEGORY_SPECIFIC
                                  L"Окно",        // CATEGORY_VIEW
                                  L"Период",      // CATEGORY_PERIOD
                                  L"Создать",     // CATEGORY_CREATE
                                  L"Правка"};     // CATEGORY_EDIT,
  assert(category >= 0 && category < _countof(titles));
  return titles[category];
}

bool CanExpandCommandCategory(CommandCategory category) {
  return category != CATEGORY_CREATE && category != CATEGORY_DEVICE &&
         category != CATEGORY_PERIOD && category != CATEGORY_NEW;
}

// Action

Action::Action(unsigned command,
               CommandCategory category,
               base::string16 title,
               base::string16 short_title,
               int image_id,
               unsigned flags)
    : command_id_(command),
      category_(category),
      title_(std::move(title)),
      short_title_(std::move(short_title)),
      image_id_(image_id),
      flags_(flags) {}

// ActionManager

ActionManager::ActionManager() {}

ActionManager::~ActionManager() {
  for (ActionMap::iterator i = action_map_.begin(); i != action_map_.end(); ++i)
    delete i->second;
}

void ActionManager::AddAction(Action& action) {
  assert(!FindAction(action.command_id()));
  action_map_[action.command_id()] = &action;
  actions_.push_back(&action);
}

Action* ActionManager::FindAction(unsigned command) const {
  ActionMap::const_iterator i = action_map_.find(command);
  return i != action_map_.end() ? i->second : NULL;
}
