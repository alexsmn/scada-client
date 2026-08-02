#pragma once

#include <string>
#include <string_view>
#include <vector>

enum class MainMenuId;

// Registry of window types that components contribute to the main menu's
// "Display" list and its per-menu "Favourites" submenus.
//
// Components (table, graph, sheet, timed data, vidicon, modus) register their
// window types here when they are constructed; the main-menu models read them
// back when a menu is about to be shown. The registry lives in the controller
// layer — below main_window — so a component can contribute without depending
// on the higher-level main_window module, which would otherwise create a
// dependency cycle (main_window links the components, the components would link
// main_window).

// Display menu: a single flat list of window types, order-preserving and
// deduplicated.
void RegisterDisplayMenuWindowType(std::string_view window_type);
void UnregisterDisplayMenuWindowType(std::string_view window_type);
const std::vector<std::string>& GetDisplayMenuWindowTypes();

// Favourites submenus, keyed by MainMenuId (Table / Graph). Each id keeps its
// own order-preserving, deduplicated list.
void RegisterMainMenuFavouritesWindowType(MainMenuId menu_id,
                                          std::string_view window_type);
void UnregisterMainMenuFavouritesWindowType(MainMenuId menu_id,
                                            std::string_view window_type);
const std::vector<std::string>& GetFavouritesMenuWindowTypes(
    MainMenuId menu_id);
