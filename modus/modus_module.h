#pragma once

#include "controller/command_registry.h"
#include "main_window/main_command_context.h"

#include <memory>

class BlinkerManager;
class ControllerRegistry;
class FileRegistry;
class ModusModule2;
class Profile;

struct ModusModuleContext {
  ControllerRegistry& controller_registry_;
  BlinkerManager& blinker_manager_;
  FileRegistry& file_registry_;
  BasicCommandRegistry<MainCommandContext> main_commands_;
  Profile& profile_;
};

class ModusModule : private ModusModuleContext {
 public:
  explicit ModusModule(ModusModuleContext&& context);
  ~ModusModule();

 private:
  std::unique_ptr<ModusModule2> modus_module_;
};
