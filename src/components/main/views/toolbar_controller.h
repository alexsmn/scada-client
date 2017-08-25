#pragma once

#include <map>

#include "base/timer/timer.h"

namespace views {
class Toolbar;
}

class ActionManager;
class CommandHandler;

class ToolbarController {
 public:
  ToolbarController(views::Toolbar& toolbar, CommandHandler& command_handler, ActionManager& action_manager);

  void UpdateCommands();
  void UpdateCommandStates();

  void AddImage(unsigned resource_id);

 private:
  int GetImageIndex(unsigned command_id) const;

  views::Toolbar& toolbar_;
  CommandHandler& command_handler_;
  ActionManager& action_manager_;

  std::map<unsigned /*command_id*/, int /*image_index*/> image_indexes_;

  base::RepeatingTimer idle_timer_;
};