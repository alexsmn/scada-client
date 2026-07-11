#pragma once

// Identifies a top-level section of the client's main menu. Declared in its own
// header so lightweight consumers (e.g. the main-menu window-type registry) can
// depend on the enum without pulling in the full command/action machinery that
// command_ui_registry.h brings along.
enum class MainMenuId {
  Display,
  Table,
  Graph,
  Item,
  More,
  Page,
  Window,
  Settings,
  Language,
  Help,
};
