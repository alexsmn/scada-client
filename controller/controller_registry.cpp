#include "controller_registry.h"

#include <cassert>
#include <map>

#include "base/lazy_instance.h"

namespace {

typedef std::map<unsigned /*command_id*/, ControllerRegistrarBase*>
    ControllerRegistrarMap;

base::LazyInstance<ControllerRegistrarMap>::Leaky g_controller_registrar_map =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

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
