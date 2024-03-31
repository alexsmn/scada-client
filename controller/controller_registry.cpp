#include "controller_registry.h"

#include "base/lazy_instance.h"

#include <cassert>

namespace {

ControllerRegistry* g_controller_registry = nullptr;

typedef std::map<unsigned /*command_id*/, ControllerRegistrarBase*>
    ControllerRegistrarMap;

base::LazyInstance<ControllerRegistrarMap>::Leaky g_static_controllers =
    LAZY_INSTANCE_INITIALIZER;

class ControllerFactoryRegistrar final : public ControllerRegistrarBase {
 public:
  ControllerFactoryRegistrar(const WindowInfo& window_info,
                             ControllerRegistryFactory controller_factory)
      : ControllerRegistrarBase{window_info, /*is_static=*/false},
        controller_factory_{std::move(controller_factory)} {}

  virtual std::unique_ptr<Controller> CreateController(
      const ControllerContext& context) override {
    return controller_factory_(context);
  }

 private:
  const ControllerRegistryFactory controller_factory_;
};

}  // namespace

// ControllerRegistry

ControllerRegistry::ControllerRegistry() {
  assert(!g_controller_registry);
  g_controller_registry = this;
}

ControllerRegistry::~ControllerRegistry() {
  assert(g_controller_registry == this);
  g_controller_registry = nullptr;
}

void ControllerRegistry::AddControllerFactory(
    const WindowInfo& window_info,
    ControllerRegistryFactory controller_factory) {
  // Intentionally leaked.
  auto* registrar =
      new ControllerFactoryRegistrar(window_info, controller_factory);

  assert(g_controller_registry);
  auto& registrars = g_controller_registry->registrars_;
  assert(!registrars.contains(window_info.command_id));
  registrars.try_emplace(window_info.command_id, registrar);
}

ControllerRegistryFactory ControllerRegistry::GetControllerFactory(
    unsigned command_id) const {
  ControllerRegistrarBase* registrar = GetControllerRegistrar(command_id);
  if (!registrar) {
    return nullptr;
  }
  return std::bind_front(&ControllerRegistrarBase::CreateController, registrar);
}

// ControllerRegistrarBase

ControllerRegistrarBase::ControllerRegistrarBase(const WindowInfo& window_info,
                                                 bool is_static)
    : window_info_{window_info} {
  if (is_static) {
    auto& registrar = g_static_controllers.Get();
    assert(registrar.find(window_info.command_id) == registrar.end());
    registrar.emplace(window_info.command_id, this);
  }
}

ControllerRegistrarBase* GetControllerRegistrar(unsigned command_id) {
  if (const ControllerRegistrarMap* static_map =
          g_static_controllers.Pointer()) {
    if (auto i = static_map->find(command_id); i != static_map->end()) {
      return i->second;
    }
  }

  // Can be null in tests.
  if (g_controller_registry) {
    const auto& registrars = g_controller_registry->registrars_;
    if (auto i = registrars.find(command_id); i != registrars.end()) {
      return i->second;
    }
  }

  return nullptr;
}

ControllerRegistrarBase* FindControllerRegistrar(std::string_view name) {
  for (const auto& [_, registrar] : g_static_controllers.Get()) {
    if (registrar->window_info().name == name) {
      return registrar;
    }
  }

  assert(g_controller_registry);
  for (const auto& [_, registrar] : g_controller_registry->registrars_) {
    if (registrar->window_info().name == name) {
      return registrar;
    }
  }

  return nullptr;
}
