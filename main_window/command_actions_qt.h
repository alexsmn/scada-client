#pragma once

#include "controller/action_manager.h"

#include <boost/signals2/connection.hpp>
#include <cstddef>
#include <functional>
#include <map>

class CommandHandler;
class CommandManager;
class QAction;
class QMenu;
class QObject;

// The main window's QAction for every command that generic button surfaces
// may carry (`CommandDescriptor::show_in_toolbar`), with the state each one
// takes from the command handler the window currently resolves.
//
// It is built from the command catalog alone and belongs to no widget. The
// command toolbar is one consumer: it lays these actions out, grouped by
// category, but does not create them. Until 2026-09-27 it did — the actions
// and the maps finding them were a side effect of building the toolbar, so
// the toolbar could not be removed without losing the only QAction set the
// window keeps in step with its handlers (backlog 697).
//
// Each action is created hidden. `UpdateAll` shows the ones whose command
// resolves a handler, and tracks that handler's enabled and checked state;
// triggering an action re-resolves its handler and runs the command only if it
// is still enabled, so a stale enabled flag can never execute anything.
class CommandActions {
 public:
  // Returns the handler that would execute `command_id` in the window's
  // current context, or null when none does.
  using HandlerResolver = std::function<CommandHandler*(unsigned command_id)>;

  // Creates one QAction, parented to `parent`, per catalog command that asks
  // to be on a generic button surface, and starts following `action_manager`'s
  // change notifications. Both managers must outlive this object.
  CommandActions(const CommandManager& command_manager,
                 ActionManager& action_manager,
                 HandlerResolver resolve_handler,
                 QObject& parent);
  ~CommandActions();

  CommandActions(const CommandActions&) = delete;
  CommandActions& operator=(const CommandActions&) = delete;

  // The action for `command_id`, or null when the command has none.
  QAction* Find(unsigned command_id) const;

  // How many commands have an action.
  std::size_t size() const { return actions_.size(); }

  // Re-resolves every action's handler and applies its visibility, enabled
  // and checked state. Titles are left alone; they change only through the
  // action manager's notifications. Call it whenever the context that commands
  // resolve against may have moved (the selection, the active view).
  void UpdateAll();

  // As UpdateAll, including titles, for the actions in `menu` that are ours.
  // Anything else in the menu is left untouched.
  void UpdateMenu(QMenu& menu);

 private:
  void Update(QAction& action, unsigned command_id, ActionChangeMask mask);
  void OnActionChanged(Action& action, ActionChangeMask change_mask);

  ActionManager& action_manager_;
  const HandlerResolver resolve_handler_;

  std::map<unsigned /*command_id*/, QAction*> actions_;
  std::map<QAction*, unsigned /*command_id*/> command_ids_;

  boost::signals2::scoped_connection action_changed_connection_;
};
