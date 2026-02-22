#include "main_window/action_manager.h"

#include "aui/translation.h"
#include "common_resources.h"
#include "main_window/action.h"

GroupedActions GroupCommands(ActionManager& action_manager,
                             const std::vector<unsigned>& commands) {
  GroupedActions grouped_commands;
  for (unsigned command_id : commands) {
    if (auto* action = action_manager.FindAction(command_id))
      grouped_commands[action->category_].push_back(action);
  }
  return grouped_commands;
}

std::u16string GetCommandCategoryTitle(CommandCategory category) {
  static const char* const kTitles[] = {
      "New",        // CATEGORY_NEW
      "Open",       // CATEGORY_OPEN
      "Item",       // CATEGORY_ITEM
      "Device",     // CATEGORY_DEVICE
      "Options",    // CATEGORY_SETUP
      "Export",     // CATEGORY_EXPORT
      "Misc",       // CATEGORY_SPECIFIC
      "Window",     // CATEGORY_VIEW
      "Period",     // CATEGORY_PERIOD
      "Create",     // CATEGORY_CREATE
      "Edit",       // CATEGORY_EDIT,
      "Function",   // CATEGORY_AGGREGATION
      "Interval",   // CATEGORY_INTERVAL
  };
  static_assert(std::size(kTitles) == static_cast<size_t>(CATEGORY_COUNT));
  assert(category >= 0 && category < _countof(kTitles));
  return Translate(kTitles[category]);
}

bool CanExpandCommandCategory(CommandCategory category) {
  return category != CATEGORY_CREATE && category != CATEGORY_DEVICE &&
         category != CATEGORY_PERIOD && category != CATEGORY_NEW &&
         category != CATEGORY_AGGREGATION && category != CATEGORY_INTERVAL &&
         category != CATEGORY_EXPORT;
}

// ActionManager

ActionManager::ActionManager() {}

ActionManager::~ActionManager() {
  for (ActionMap::iterator i = action_map_.begin(); i != action_map_.end(); ++i)
    delete i->second;
}

Action& ActionManager::AddAction(Action& action) {
  assert(!FindAction(action.command_id()));
  action_map_[action.command_id()] = &action;
  actions_.push_back(&action);
  return action;
}

Action* ActionManager::FindAction(unsigned command) const {
  ActionMap::const_iterator i = action_map_.find(command);
  return i != action_map_.end() ? i->second : NULL;
}

void ActionManager::Subscribe(ActionObserver& observer) {
  observers_.AddObserver(&observer);
}

void ActionManager::Unsubscribe(ActionObserver& observer) {
  observers_.RemoveObserver(&observer);
}

void ActionManager::NotifyActionChanged(unsigned command_id,
                                        ActionChangeMask change_mask) {
  auto* action = FindAction(command_id);
  if (!action)
    return;

  for (auto& obs : observers_)
    obs.OnActionChanged(*action, change_mask);
}
