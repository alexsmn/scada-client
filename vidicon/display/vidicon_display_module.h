#pragma once

class ControllerRegistry;
class WriteService;

struct VidiconDisplayModuleContext {
  ControllerRegistry& controller_registry_;
  WriteService& write_service_;
};

class VidiconDisplayModule : private VidiconDisplayModuleContext {
 public:
  explicit VidiconDisplayModule(VidiconDisplayModuleContext&& context);
};
