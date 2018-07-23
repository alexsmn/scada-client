#pragma once

#include "components/main/action.h"

#include <map>
#include <vector>

using ActionList = std::vector<Action*>;

class ActionManager {
 public:
  typedef std::map<unsigned, Action*> ActionMap;

  ActionManager();
  ~ActionManager();

  ActionManager(const ActionManager&) = delete;
  ActionManager& operator=(const ActionManager&) = delete;

  const ActionList& actions() const { return actions_; }

  void AddAction(Action& action);
  Action* FindAction(unsigned command) const;

 private:
  ActionMap action_map_;
  ActionList actions_;
};

typedef std::map<CommandCategory, ActionList> GroupedActions;

GroupedActions GroupCommands(ActionManager& action_manager,
                             const std::vector<unsigned>& commands);

const base::char16* GetCommandCategoryTitle(CommandCategory category);
bool CanExpandCommandCategory(CommandCategory category);
