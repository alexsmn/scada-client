#pragma once

#include "controller.h"

#include <functional>
#include <memory>

class SelectionCommands;
struct WindowInfo;

class ComponentApi {
 public:
  virtual ~ComponentApi() = default;

  template <class ControllerClass>
  void RegisterController(const WindowInfo& window_info);

  using ControllerFactory = std::function<std::unique_ptr<Controller>()>;

  virtual void RegisterControllerFactory(
      const WindowInfo& window_info,
      ControllerFactory controller_factory) = 0;

  virtual SelectionCommands& GetSelectionCommands() = 0;
};

template <class ControllerClass>
void ComponentApi::RegisterController(const WindowInfo& window_info) {
  RegisterControllerFactory(window_info,
                            [] { return std::make_unique<ControllerClass>(); });
}

#define REGISTER_COMPONENT(ComponentClass)                             \
  static bool k##ComponentClass##Registered = [] {                    \
    GetComponentRegistry().AddComponentFactory([](ComponentApi& api) { \
      return std::make_unique<ComponentClass>(api)                     \
    });                                                                \
    return true;                                                       \
  }();
