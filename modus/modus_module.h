#pragma once

#include <memory>

class BlinkerManager;
class ControllerRegistry;
class FileRegistry;
class ModusModule2;

struct ModusModuleContext {
  ControllerRegistry& controller_registry_;
  BlinkerManager& blinker_manager_;
  FileRegistry& file_registry_;
};

class ModusModule : private ModusModuleContext {
 public:
  explicit ModusModule(ModusModuleContext&& context);
  ~ModusModule();

 private:
  std::unique_ptr<ModusModule2> modus_module_;
};
