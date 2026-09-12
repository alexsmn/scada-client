#pragma once

#include "base/any_executor.h"

#include "base/awaitable.h"
#include "controller/command_ui_registry.h"
#include "profile/window_definition.h"

#include <functional>
#include <vector>

class MainWindowInterface;
struct NodeCommandContext;

Awaitable<void> OpenView(MainWindowInterface* main_window,
                         const WindowDefinition& window_def,
                         bool activate = true);

bool ExecuteDefaultNodeCommand(const AnyExecutor& executor,
                               const NodeCommandContext& context);

// The menu contributions that open a view with **no selection at all** — the
// "Empty" group of the workspace tab strip's `+`
// (docs/product/ui-mockups/screens/shell-chrome.html). These are the commands
// whose only home in the shell was the menu bar.
//
// Derived rather than listed. A contribution under the Graph or Table menu
// whose command id is a registered *window* id is one that opens a view, which
// is exactly what makes it an empty-view opener — so a view that adds one is
// picked up without an edit here. `Group Table` sits in the same menu and is
// **not** one: it is a selection command over the parent group, and its command
// id names no window.
//
// `is_enabled` is asked per surviving contribution, so a command the session
// cannot currently run is left out rather than offered dead. The caller owns
// command resolution, which is why this takes a predicate instead of reaching
// for it.
std::vector<MenuContribution> FindEmptyViewCommands(
    const UiCommandRegistry& ui_command_registry,
    const std::function<bool(unsigned command_id)>& is_enabled);
