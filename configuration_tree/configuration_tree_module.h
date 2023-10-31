#pragma once

class ControllerRegistry;
class Profile;

struct ConfigurationTreeModuleContext {
  ControllerRegistry& controller_registry_;
  Profile& profile_;
};

class ConfigurationTreeModule : private ConfigurationTreeModuleContext {
 public:
  explicit ConfigurationTreeModule(ConfigurationTreeModuleContext&& context);
};