#include "controller/component_api_impl.h"

void ComponentApiImpl::RegisterControllerFactory(
    const WindowInfo& window_info,
    ControllerFactory controller_factory) {}

SelectionCommands& ComponentApiImpl::GetSelectionCommands() {
  return *selection_commands_;
}
