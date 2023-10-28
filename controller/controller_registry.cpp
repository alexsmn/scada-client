#include "controller_registry.h"

#include <cassert>
#include <map>

#include "base/lazy_instance.h"

namespace {

typedef std::map<unsigned /*command_id*/, ControllerRegistrarBase*>
    ControllerRegistrarMap;

base::LazyInstance<ControllerRegistrarMap>::Leaky g_controller_registrar_map =
    LAZY_INSTANCE_INITIALIZER;

class ControllerFactoryRegistrar final : public ControllerRegistrarBase {
 public:
  ControllerFactoryRegistrar(const WindowInfo& window_info,
                             ControllerRegistryFactory controller_factory)
      : ControllerRegistrarBase{window_info},
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

void ControllerRegistry::AddControllerFactory(
    const WindowInfo& window_info,
    ControllerRegistryFactory controller_factory) {
  // Intentionally leaked.
  new ControllerFactoryRegistrar(window_info, controller_factory);
}

// ControllerRegistrarBase

ControllerRegistrarBase::ControllerRegistrarBase(const WindowInfo& window_info)
    : window_info_{window_info} {
  auto& registrar = g_controller_registrar_map.Get();
  assert(registrar.find(window_info.command_id) == registrar.end());
  registrar.emplace(window_info.command_id, this);
}

ControllerRegistrarBase* GetControllerRegistrar(unsigned command_id) {
  assert(g_controller_registrar_map.Pointer());
  auto& map = g_controller_registrar_map.Get();
  auto i = map.find(command_id);
  return i == map.end() ? nullptr : i->second;
}

ControllerRegistrarBase* FindControllerRegistrar(std::string_view name) {
  for (const auto& p : g_controller_registrar_map.Get()) {
    auto& registrar = *p.second;
    if (registrar.window_info().name == name)
      return &registrar;
  }
  return nullptr;
}
