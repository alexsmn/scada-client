#include "controller/window_info.h"

#include "controller/controller_registry.h"

#include "base/check.h"

const WindowInfo* FindWindowInfo(unsigned command_id) {
  auto* registrar = GetControllerRegistrar(command_id);
  return registrar ? &registrar->window_info() : nullptr;
}

const WindowInfo& GetWindowInfo(unsigned command_id) {
  const WindowInfo* info = FindWindowInfo(command_id);
  base::Check(info);
  return *info;
}

const WindowInfo* FindWindowInfoByName(std::string_view name) {
  auto* registrar = FindControllerRegistrar(name);
  return registrar ? &registrar->window_info() : nullptr;
}
