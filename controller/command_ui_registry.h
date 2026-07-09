#pragma once

#include "base/lifetime.h"
#include "controller/action_manager.h"
#include "controller/command_manager.h"

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
  Language,
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

// Describes where a command should appear in generated UI surfaces.
struct CommandPlacements {
  std::vector<MenuContribution> main_menu;
  bool toolbar = true;
  bool context_menu = true;
};

class UiCommandRegistry {
 public:
  ActionManager& action_manager() SCADA_LIFETIME_BOUND {
    return action_manager_;
  }
  const ActionManager& action_manager() const SCADA_LIFETIME_BOUND {
    return action_manager_;
  }

  CommandManager& command_manager() SCADA_LIFETIME_BOUND {
    return command_manager_;
  }
  const CommandManager& command_manager() const SCADA_LIFETIME_BOUND {
    return command_manager_;
  }

  // Registers command metadata and UI placements through the unified command
  // catalog.
  CommandDescriptor& RegisterCommand(
      CommandDescriptor descriptor,
      CommandPlacements placements = CommandPlacements{});

  // Registers a command handler for context-aware command resolution.
  void RegisterHandler(unsigned command_id,
                       CommandContextId context_id,
                       CommandHandler& handler);

  void AddAction(Action action);
  void AddMenuItem(MenuContribution contribution);

  std::vector<MenuContribution> GetMenuContributions(MainMenuId menu_id) const;

 private:
  ActionManager action_manager_;
  CommandManager command_manager_;
  std::vector<MenuContribution> menu_contributions_;
};
