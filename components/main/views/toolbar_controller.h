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
  ToolbarController(ActionManager& action_manager,
                    views::Toolbar& toolbar,
                    CommandHandler& command_handler);

  void UpdateCommands();
  void UpdateCommandStates();

 private:
  int GetImageIndex(unsigned command_id) const;

  ActionManager& action_manager_;
  views::Toolbar& toolbar_;
  CommandHandler& command_handler_;

  std::map<unsigned /*command_id*/, int /*image_index*/> image_indexes_;

  base::RepeatingTimer idle_timer_;
};
