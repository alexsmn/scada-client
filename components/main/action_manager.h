#pragma once

#include "base/observer_list.h"
#include "components/main/action.h"

#include <map>
#include <vector>

using ActionList = std::vector<Action*>;

class ActionObserver {
 public:
  virtual ~ActionObserver() {}

  virtual void OnActionUpdated(Action& action) = 0;
};

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

  void Subscribe(ActionObserver& observer);
  void Unsubscribe(ActionObserver& observer);

  void NotifyActionUpdated(unsigned command_id);

 private:
  ActionMap action_map_;
  ActionList actions_;
  base::ObserverList<ActionObserver> observers_;
};

typedef std::map<CommandCategory, ActionList> GroupedActions;

GroupedActions GroupCommands(ActionManager& action_manager,
                             const std::vector<unsigned>& commands);

const base::char16* GetCommandCategoryTitle(CommandCategory category);
bool CanExpandCommandCategory(CommandCategory category);
