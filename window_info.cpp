#include "window_info.h"

#include <cassert>
#include <cstdlib>
#include <exception>

#include "base/compiler_specific.h"
#include "common_resources.h"
#include "controller_factory.h"

const WindowInfo* FindWindowInfo(unsigned command_id) {
  auto* registrar = GetControllerRegistrar(command_id);
  return registrar ? &registrar->window_info() : nullptr;
}

const WindowInfo& GetWindowInfo(unsigned command_id) {
  const WindowInfo* info = FindWindowInfo(command_id);
  assert(info);
  return *info;
}

unsigned ParseWindowType(std::string_view str) {
  auto* registrar = FindControllerRegistrar(str);
  return registrar ? registrar->window_info().command_id : 0;
}

const char* ViewTypeToString(unsigned command_id) {
  auto* window_info = FindWindowInfo(command_id);
  if (!window_info)
    return "Unknown";
  return window_info->name;
}
