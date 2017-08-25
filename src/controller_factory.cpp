#include "controller_factory.h"

#include <cassert>
#include <map>

#include "base/lazy_instance.h"
#include "window_info.h"

namespace {

typedef std::map<unsigned /*command_id*/, ControllerRegistrarBase*> ControllerRegistrarMap;

base::LazyInstance<ControllerRegistrarMap>::Leaky g_controller_registrar_map =
    LAZY_INSTANCE_INITIALIZER; 

} // namespace

ControllerRegistrarBase::ControllerRegistrarBase(unsigned command_id) {
  auto& registrar = g_controller_registrar_map.Get();
  assert(registrar.find(command_id) == registrar.end());
  registrar.emplace(command_id, this);
}

std::unique_ptr<Controller> CreateController(unsigned command_id, const ControllerContext& context) {
  assert(g_controller_registrar_map.Pointer());
  auto& registrar = g_controller_registrar_map.Get();
  auto i = registrar.find(command_id);
  return i == registrar.end() ? nullptr : i->second->CreateController(context);
}
