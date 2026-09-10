#pragma once

#include "base/lifetime.h"
#include "controller/action.h"

#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>
#include <functional>
#include <map>
#include <vector>

using ActionList = std::vector<Action*>;

enum class ActionChangeMask : unsigned {
  Title = 0x01,
  Visible = 0x02,
  Enabled = 0x04,
  Checked = 0x08,
  All = 0xFF,
  AllButTitle = All & ~Title,
};

class ActionManager {
 public:
  typedef std::map<unsigned, Action> ActionMap;

  ActionManager();
  ~ActionManager();

  ActionManager(const ActionManager&) = delete;
  ActionManager& operator=(const ActionManager&) = delete;

  const ActionList& actions() const SCADA_LIFETIME_BOUND { return actions_; }

  Action& AddAction(Action action);
  Action* FindAction(unsigned command) const;

  using ActionChangedCallback =
      std::function<void(Action& action, ActionChangeMask change_mask)>;

  // Notifies after an action state (title/visibility/enabled/checked)
  // changed.
  [[nodiscard]] boost::signals2::scoped_connection Subscribe(
      const ActionChangedCallback& callback);

  void NotifyActionChanged(
      unsigned command_id,
      ActionChangeMask change_mask = ActionChangeMask::AllButTitle);

 private:
  ActionMap action_map_;
  ActionList actions_;
  boost::signals2::signal<void(Action&, ActionChangeMask)>
      action_changed_signal_;
};

typedef std::map<CommandCategory, ActionList> GroupedActions;

GroupedActions GroupCommands(ActionManager& action_manager,
                             const std::vector<unsigned>& commands);

std::u16string GetCommandCategoryTitle(CommandCategory category);

// Where a command group is being drawn. The two surfaces deliberately group
// CATEGORY_OPEN differently, which is what this exists for --
// docs/product/ui-mockups/authoring.md 4b "Opening a view" draws the Explorer
// context menu expanding the seven Open commands inline above the item
// commands, and the toolbar collapsing them into one labelled `Open` button
// with a menu. Every other category groups the same way on both.
enum class CommandSurface {
  // The Explorer's node context menu (ContextMenuModel).
  kContextMenu,
  // The main window's command toolbar (MainWindow::CreateToolbar).
  kToolbar,
};

// Whether `category`'s commands are drawn inline on `surface`, rather than
// collapsed behind one button or submenu carrying the category's title.
bool CanExpandCommandCategory(CommandCategory category, CommandSurface surface);
