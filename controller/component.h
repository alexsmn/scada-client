#pragma once

#include <functional>
#include <memory>

class ComponentRegistry {
 public:
  using ComponentFactory = std::function<std::unique_ptr<Component>()>;

  void RegisterComponentFactory(ComponentFactory component_factory);
};

#define REGISTER_COMPONENT(ComponentClass)                                  \
  static bool k##ComponentClass##Registered = [] {                          \
    GetComponentRegistry().RegisterComponentFactory([](ComponentApi& api) { \
      return std::make_unique<ComponentClass>(api)                          \
    });                                                                     \
    return true;                                                            \
  }();
