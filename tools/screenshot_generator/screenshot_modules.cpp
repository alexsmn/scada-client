#include "screenshot_modules.h"

#include "configuration/configuration_module.h"
#include "services/ui/node_service_progress_tracker.h"

ClientApplicationModuleConfigurator MakeScreenshotModules() {
  return [](ClientApplicationModuleContext& context) {
    context.singletons_.emplace(std::make_shared<ConfigurationModule>(
        ConfigurationModuleContext{
            .controller_registry_ = context.controller_registry_,
            .profile_ = context.profile_,
            .node_service_tree_factory_ = context.node_service_tree_factory_}));

    context.singletons_.emplace(std::make_shared<NodeServiceProgressTracker>(
        context.executor_, context.node_service_, context.progress_host_));
  };
}
