#pragma once

class ControllerRegistry;
class Profile;

struct ConfigurationModuleContext {
  ControllerRegistry& controller_registry_;
  Profile& profile_;
};

class ConfigurationModule : private ConfigurationModuleContext {
 public:
  explicit ConfigurationModule(ConfigurationModuleContext&& context);
};