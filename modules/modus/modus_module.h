#pragma once

#include "controller/command_registry.h"
#include "core/global_command_context.h"

class BlinkerManager;
class ControllerRegistry;
class FileRegistry;
class Profile;
class UiCommandRegistry;

struct ModusModuleContext {
  ControllerRegistry& controller_registry_;
  BlinkerManager& blinker_manager_;
  FileRegistry& file_registry_;
  BasicCommandRegistry<GlobalCommandContext>& global_commands_;
  UiCommandRegistry& ui_command_registry_;
  Profile& profile_;
};

class ModusModule : private ModusModuleContext {
 public:
  explicit ModusModule(ModusModuleContext&& context);
  ~ModusModule();
};
