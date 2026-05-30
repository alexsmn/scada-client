#pragma once

#include "controller/action_manager.h"

#include <optional>
#include <string>
#include <vector>

enum class MainMenuId {
  Display,
  Table,
  Graph,
  Item,
  More,
  Page,
  Window,
  Settings,
  Help,
};

struct MenuContribution {
  MainMenuId menu_id = MainMenuId::More;
  int order = 0;
  unsigned command_id = 0;
  std::u16string title;
  bool checkable = false;
  bool separator_before = false;
  bool admin_only = false;
  bool debug_only = false;
};

class UiCommandRegistry {
 public:
  ActionManager& action_manager() { return action_manager_; }
  const ActionManager& action_manager() const { return action_manager_; }

  void AddAction(Action action);
  void AddMenuItem(MenuContribution contribution);

  std::vector<MenuContribution> GetMenuContributions(MainMenuId menu_id) const;

 private:
  ActionManager action_manager_;
  std::vector<MenuContribution> menu_contributions_;
};
