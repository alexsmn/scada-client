#include "controller/window_info.h"

#include "controller/controller_registry.h"

#include <cassert>

const WindowInfo* FindWindowInfo(unsigned command_id) {
  auto* registrar = GetControllerRegistrar(command_id);
  return registrar ? &registrar->window_info() : nullptr;
}

const WindowInfo& GetWindowInfo(unsigned command_id) {
  const WindowInfo* info = FindWindowInfo(command_id);
  assert(info);
  return *info;
}

const WindowInfo* FindWindowInfoByName(std::string_view name) {
  auto* registrar = FindControllerRegistrar(name);
  return registrar ? &registrar->window_info() : nullptr;
}
