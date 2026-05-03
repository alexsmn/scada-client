#pragma once

#include "common/aliases.h"
#include "core/global_command_context.h"

#include <memory>

class ActionManager;
class BlinkerManager;
class ControllerRegistry;
class FileRegistry;
class ModusModule2;
class Profile;

struct ModusModuleContext {
  ControllerRegistry& controller_registry_;
  BlinkerManager& blinker_manager_;
  FileRegistry& file_registry_;
  ActionManager& action_manager_;
  Profile& profile_;
  AliasResolver alias_resolver_;
};

class ModusModule : private ModusModuleContext {
 public:
  explicit ModusModule(ModusModuleContext&& context);
  ~ModusModule();

 private:
  std::unique_ptr<ModusModule2> modus_module_;
};
