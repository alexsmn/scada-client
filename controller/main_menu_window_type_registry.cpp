#include "controller/main_menu_window_type_registry.h"

#include "base/check.h"
#include "controller/main_menu_id.h"

#include <algorithm>

namespace {

std::vector<std::string>& MutableDisplayMenuWindowTypes() {
  static std::vector<std::string> window_types;
  return window_types;
}

std::vector<std::string>& MutableFavouritesMenuWindowTypes(MainMenuId menu_id) {
  static std::vector<std::string> table_window_types;
  static std::vector<std::string> graph_window_types;

  switch (menu_id) {
    case MainMenuId::Table:
      return table_window_types;
    case MainMenuId::Graph:
      return graph_window_types;
    default:
      scada::base::NotReached();
  }
}

}  // namespace

void RegisterDisplayMenuWindowType(std::string_view window_type) {
  auto& window_types = MutableDisplayMenuWindowTypes();
  if (std::ranges::find(window_types, window_type) == window_types.end()) {
    window_types.emplace_back(window_type);
  }
}

void UnregisterDisplayMenuWindowType(std::string_view window_type) {
  std::erase(MutableDisplayMenuWindowTypes(), window_type);
}

const std::vector<std::string>& GetDisplayMenuWindowTypes() {
  return MutableDisplayMenuWindowTypes();
}

void RegisterMainMenuFavouritesWindowType(MainMenuId menu_id,
                                          std::string_view window_type) {
  auto& window_types = MutableFavouritesMenuWindowTypes(menu_id);
  if (std::ranges::find(window_types, window_type) == window_types.end()) {
    window_types.emplace_back(window_type);
  }
}

void UnregisterMainMenuFavouritesWindowType(MainMenuId menu_id,
                                            std::string_view window_type) {
  std::erase(MutableFavouritesMenuWindowTypes(menu_id), window_type);
}

const std::vector<std::string>& GetFavouritesMenuWindowTypes(
    MainMenuId menu_id) {
  return MutableFavouritesMenuWindowTypes(menu_id);
}
