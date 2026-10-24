#include "main_window/command_shortcuts_qt.h"

#include "aui/qt/key_codes.h"
#include "controller/command_handler.h"
#include "controller/command_manager.h"

#include <QShortcut>
#include <QWidget>

#include <utility>

CommandShortcuts::CommandShortcuts(const CommandManager& command_manager,
                                   HandlerResolver resolve_handler,
                                   QWidget& window)
    : resolve_handler_{std::move(resolve_handler)} {
  for (const CommandDescriptor* command : command_manager.commands()) {
    if (!command->shortcut.has_value())
      continue;

    auto* shortcut =
        new QShortcut(scada::aui::ToQKeySequence(command->shortcut->modifiers(),
                                                 command->shortcut->key_code()),
                      &window);
    shortcut->setContext(Qt::WindowShortcut);
    // A held Delete would otherwise queue one delete flow per repeat.
    shortcut->setAutoRepeat(false);
    // Disabled until the first Refresh: nothing resolves before the window has
    // an active view, and a disabled shortcut leaves the key to the widget.
    shortcut->setEnabled(false);

    const unsigned command_id = command->command_id;
    QObject::connect(shortcut, &QShortcut::activated, shortcut,
                     [this, command_id] { Activate(command_id); });
    shortcuts_.emplace(command_id, shortcut);
  }
}

CommandShortcuts::~CommandShortcuts() {
  // The shortcuts belong to the window, which outlives this registry, and
  // their handlers capture `this`; deleting them here is what keeps a late key
  // press from reaching a destroyed registry.
  for (const auto& [_, shortcut] : shortcuts_)
    delete shortcut;
}

QShortcut* CommandShortcuts::Find(unsigned command_id) const {
  auto i = shortcuts_.find(command_id);
  return i == shortcuts_.end() ? nullptr : i->second;
}

void CommandShortcuts::Refresh() {
  for (const auto& [command_id, shortcut] : shortcuts_) {
    CommandHandler* handler = resolve_handler_(command_id);
    shortcut->setEnabled(handler && handler->IsCommandEnabled(command_id));
  }
}

void CommandShortcuts::Activate(unsigned command_id) {
  // Re-resolve rather than trusting the last Refresh: the enabled state is
  // only as fresh as the last selection change, and a stale one must never
  // execute anything.
  CommandHandler* handler = resolve_handler_(command_id);
  if (handler && handler->IsCommandEnabled(command_id))
    handler->ExecuteCommand(command_id);
}
