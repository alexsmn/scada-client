#include "controller/action_manager.h"

#include "aui/translation.h"
#include "resources/common_resources.h"
#include "controller/action.h"

#include <utility>

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

ActionManager::~ActionManager() {}

namespace {

void MergeAction(Action& target, Action source) {
  if (source.category_ != CATEGORY_SPECIFIC) {
    target.category_ = source.category_;
  }
  if (!source.title_.empty()) {
    target.title_ = std::move(source.title_);
  }
  if (!source.short_title_.empty()) {
    target.short_title_ = std::move(source.short_title_);
  }
  if (source.image_id_ != 0) {
    target.image_id_ = source.image_id_;
  }
  if (source.flags_ != 0) {
    target.flags_ |= source.flags_;
  }
  if (source.shortcut_) {
    target.shortcut_ = std::move(source.shortcut_);
  }
  if (source.menu_group_) {
    target.menu_group_ = source.menu_group_;
  }
  if (source.title_provider_) {
    target.title_provider_ = std::move(source.title_provider_);
  }
  if (source.execute_handler_) {
    target.execute_handler_ = std::move(source.execute_handler_);
  }
  if (source.available_handler_) {
    target.available_handler_ = std::move(source.available_handler_);
  }
  if (source.enabled_handler_) {
    target.enabled_handler_ = std::move(source.enabled_handler_);
  }
  if (source.checked_handler_) {
    target.checked_handler_ = std::move(source.checked_handler_);
  }
}

}  // namespace

Action& ActionManager::AddAction(Action action) {
  static unsigned next_command_id = 10000;
  if (action.command_id_ == 0) {
    action.command_id_ = next_command_id++;
  }

  const auto command_id = action.command_id();
  if (auto* existing_action = FindAction(command_id)) {
    MergeAction(*existing_action, std::move(action));
    return *existing_action;
  }

  auto [it, inserted] = action_map_.emplace(command_id, std::move(action));
  assert(inserted);
  actions_.push_back(&it->second);
  return it->second;
}

Action* ActionManager::FindAction(unsigned command) const {
  ActionMap::const_iterator i = action_map_.find(command);
  return i != action_map_.end() ? const_cast<Action*>(&i->second) : NULL;
}

bool ActionManager::IsActionAvailable(unsigned command_id,
                                      ActionContext context) const {
  const auto* action = FindAction(command_id);
  return action && (!action->available_handler_ ||
                    action->available_handler_(context));
}

bool ActionManager::IsActionEnabled(unsigned command_id,
                                    ActionContext context) const {
  const auto* action = FindAction(command_id);
  return action && (!action->enabled_handler_ ||
                    action->enabled_handler_(context));
}

bool ActionManager::IsActionChecked(unsigned command_id,
                                    ActionContext context) const {
  const auto* action = FindAction(command_id);
  return action && action->checked_handler_ &&
         action->checked_handler_(context);
}

void ActionManager::ExecuteAction(unsigned command_id,
                                  ActionContext context) const {
  const auto* action = FindAction(command_id);
  if (action && action->execute_handler_) {
    action->execute_handler_(context);
  }
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
