#pragma once

class QMenuBar;

namespace scada::aui {
class MenuModel;
}

// The main window's menu bar, built from the main menu model: one top-level
// menu per submenu of the model, in model order.
//
// Split out of MainWindow on 2026-10-04 (backlog 722). Free functions rather
// than a class because the bar holds no state of its own — what each menu
// shows is re-read from the model whenever it opens.

// Adds one top-level menu per submenu of `model` to `menu_bar`. Each menu is
// rebuilt from its submenu model every time it is about to show, so it always
// reflects the current selection and rights. `model` must outlive `menu_bar`.
void PopulateMenuBar(QMenuBar& menu_bar, scada::aui::MenuModel& model);

// Rebuilds every top-level menu from `model` now, without waiting for it to
// open — for the native macOS menu bar, which reads a menu's items before
// `aboutToShow` and would otherwise show the placeholder.
void RebuildMenuBar(QMenuBar& menu_bar, scada::aui::MenuModel& model);
