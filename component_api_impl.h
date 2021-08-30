#pragma once

#include "component_api.h"

class ComponentApiImpl : public ComponentApi {
 public:
  // ComponentApi
  virtual void RegisterControllerFactory(
      const WindowInfo& window_info,
      ControllerFactory controller_factory) override;
};
