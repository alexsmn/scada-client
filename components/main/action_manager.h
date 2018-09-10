#pragma once

#include "base/observer_list.h"
#include "components/main/action.h"

#include <map>
#include <vector>

using ActionList = std::vector<Action*>;

enum class ActionChangeMask : unsigned {
  Title = 0x01,
  Visible = 0x02,
  Enabled = 0x04,
  Checked = 0x08,
  All = 0xFF,
  AllButTitle = All & ~Title,
};

class ActionObserver {
 public:
  virtual ~ActionObserver() {}

  virtual void OnActionChanged(Action& action,
                               ActionChangeMask change_mask) = 0;
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

  void NotifyActionChanged(
      unsigned command_id,
      ActionChangeMask change_mask = ActionChangeMask::AllButTitle);

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
