#pragma once

#include "base/observer_list.h"
#include "controller/action.h"

#include <map>
#include <memory>
#include <unordered_set>
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
  typedef std::map<unsigned, Action> ActionMap;

  ActionManager();
  ~ActionManager();

  ActionManager(const ActionManager&) = delete;
  ActionManager& operator=(const ActionManager&) = delete;

  const ActionList& actions() const { return actions_; }

  Action& AddAction(Action action);
  Action& AddAction(unsigned command_id) {
    return AddAction(Action{.command_id_ = command_id});
  }
  Action* FindAction(unsigned command) const;

  bool IsActionAvailable(unsigned command_id,
                         ActionContext context = nullptr) const;
  bool IsActionEnabled(unsigned command_id,
                       ActionContext context = nullptr) const;
  bool IsActionChecked(unsigned command_id,
                       ActionContext context = nullptr) const;
  void ExecuteAction(unsigned command_id,
                     ActionContext context = nullptr) const;

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

struct ActionGroupState {
  std::unordered_set<unsigned> action_ids_;
};

class ActionGroup {
 public:
  explicit ActionGroup(ActionManager& action_manager);
  ActionGroup(ActionManager& action_manager,
              std::shared_ptr<ActionGroupState> state);

  ActionGroup WithContext(ActionContext context) const;

  void SetContext(ActionContext context) { context_ = context; }
  bool Contains(unsigned command_id) const;

  Action& AddAction(Action action);
  Action& AddAction(unsigned command_id) {
    return AddAction(Action{.command_id_ = command_id});
  }
  void AddActionId(unsigned command_id);

  Action* FindAction(unsigned command_id) const;
  bool IsActionEnabled(unsigned command_id) const;
  bool IsActionChecked(unsigned command_id) const;
  void ExecuteAction(unsigned command_id) const;

 private:
  ActionManager& action_manager_;
  std::shared_ptr<ActionGroupState> state_;
  ActionContext context_ = nullptr;
};

typedef std::map<CommandCategory, ActionList> GroupedActions;

GroupedActions GroupCommands(ActionManager& action_manager,
                             const std::vector<unsigned>& commands);

std::u16string GetCommandCategoryTitle(CommandCategory category);
bool CanExpandCommandCategory(CommandCategory category);
