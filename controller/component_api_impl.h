#pragma once

#include "controller/component_api.h"

class ComponentApiImpl : public ComponentApi {
 public:
  // ComponentApi
  virtual void RegisterControllerFactory(
      const WindowInfo& window_info,
      ControllerFactory controller_factory) override;
  virtual SelectionCommands& GetSelectionCommands() override;

  const std::shared_ptr<SelectionCommands> selection_commands_;
};
