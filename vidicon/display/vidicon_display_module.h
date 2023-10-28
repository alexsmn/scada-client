#pragma once

class ControllerRegistry;

struct VidiconDisplayModuleContext {
  ControllerRegistry& controller_registry_;
};

class VidiconDisplayModule : private VidiconDisplayModuleContext {
 public:
  explicit VidiconDisplayModule(VidiconDisplayModuleContext&& context);
};
