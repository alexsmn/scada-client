#pragma once

#include <cstddef>
#include <functional>
#include <map>

class CommandHandler;
class CommandManager;
class QShortcut;
class QWidget;

// The window-wide keyboard shortcuts of the catalog's commands: one
// `QShortcut` per command that declares a `CommandDescriptor::shortcut`
// (Copy, Paste and Delete today), each running its command through the
// handler the window resolves.
//
// A shortcut is enabled only while its command resolves and is enabled, so a
// key the command cannot use right now is never swallowed: it reaches the
// focused widget as if the shortcut did not exist. That is the whole point of
// keeping the enabled state current rather than checking on activation alone —
// an activated `QShortcut` has consumed the key whether or not its handler then
// does anything. A widget that handles the same key itself and must keep it
// wins by accepting the `ShortcutOverride` event: Qt's text widgets already do
// for the standard editing keys, and the aui grid and table do for Copy.
//
// Until 2026-10-04 these keys were bound only as `QAction` shortcuts on the
// grip toolbar's actions, which Qt fires only through a visible widget, so they
// worked only while the operator had turned that toolbar on (backlog 697, 899).
class CommandShortcuts {
 public:
  // Returns the handler that would execute `command_id` in the window's
  // current context, or null when none does.
  using HandlerResolver = std::function<CommandHandler*(unsigned command_id)>;

  // Creates a disabled `QShortcut`, parented to `window`, for every catalog
  // command that declares a shortcut. `command_manager` is read only here.
  CommandShortcuts(const CommandManager& command_manager,
                   HandlerResolver resolve_handler,
                   QWidget& window);
  ~CommandShortcuts();

  CommandShortcuts(const CommandShortcuts&) = delete;
  CommandShortcuts& operator=(const CommandShortcuts&) = delete;

  // The shortcut bound to `command_id`, or null when the command declares none.
  QShortcut* Find(unsigned command_id) const;

  // How many commands have a shortcut.
  std::size_t size() const { return shortcuts_.size(); }

  // Re-resolves every command and enables exactly the shortcuts whose command
  // resolves and is enabled. Call it whenever the context commands resolve
  // against may have moved — the selection, the active view, the page.
  void Refresh();

 private:
  void Activate(unsigned command_id);

  const HandlerResolver resolve_handler_;
  std::map<unsigned /*command_id*/, QShortcut*> shortcuts_;
};
