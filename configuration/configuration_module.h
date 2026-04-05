#pragma once

#include "configuration/tree/node_service_tree.h"
#include "configuration/tree/node_service_tree_impl.h"

#include <functional>

class ControllerRegistry;
class Profile;

using NodeServiceTreeFactory = std::function<
    std::unique_ptr<NodeServiceTree>(NodeServiceTreeImplContext&&)>;

struct ConfigurationModuleContext {
  ControllerRegistry& controller_registry_;
  Profile& profile_;
  NodeServiceTreeFactory node_service_tree_factory_;
};

class ConfigurationModule : private ConfigurationModuleContext {
 public:
  explicit ConfigurationModule(ConfigurationModuleContext&& context);
};