#pragma once

#include "base/any_executor.h"

#include <boost/signals2/connection.hpp>
#include <functional>
#include <memory>
#include <string>

class CommandHandler;
class CommandManager;
class CommandPalette;
class NodeService;
class QString;
class QWidget;
class TagSearchIndex;

namespace scada {
class NodeId;
class SessionService;
}  // namespace scada

// What CommandPaletteLauncher needs from the window that hosts the palette.
struct CommandPaletteLauncherContext {
  AnyExecutor executor_;
  // Every registered command the palette lists.
  const CommandManager& command_manager_;
  // The handler the window would run `command_id` through, or null.
  std::function<CommandHandler*(unsigned command_id)> resolve_command_;
  // Opens an address-space tag the operator picked from the palette.
  std::function<void(const scada::NodeId& node_id, const std::u16string& title)>
      open_tag_;
  // The palette's parent.
  QWidget& parent_;
  // Optional: the address space, for the palette's tag search. Null gives a
  // palette of commands alone.
  NodeService* node_service_ = nullptr;
  // Optional: watched only to notice a re-login, which drops the tag index.
  scada::SessionService* session_service_ = nullptr;
};

// The Ctrl-K command palette's host: opens the palette over every registered
// command plus the address-space tags, and owns the tag index those come from.
//
// Split out of MainWindow on 2026-10-04 (backlog 722).
class CommandPaletteLauncher {
 public:
  explicit CommandPaletteLauncher(CommandPaletteLauncherContext&& context);
  ~CommandPaletteLauncher();

  CommandPaletteLauncher(const CommandPaletteLauncher&) = delete;
  CommandPaletteLauncher& operator=(const CommandPaletteLauncher&) = delete;

  // Starts the tag index's one-time address-space browse, if there is an
  // index and it has not run since the last invalidation. Cheap to repeat.
  //
  // Deliberately not started at construction. The browse walks the Organizes
  // hierarchy under ObjectsFolder one node per await, and starting it before
  // any pane existed put a continuous stream of Read/Browse round trips on the
  // session for the whole of the operator's first wait — the one the
  // Explorer's trees are already spending on their own per-child fetch
  // (docs/client/message-loop.md §10). Speculative work goes after the surfaces
  // the operator is looking at, so the host calls this once the first page is
  // open. Show() starts it too, so the palette works on its own terms.
  void StartTagSearchBrowse();

  // Opens the palette, seeded with `initial_text` when the operator started
  // typing elsewhere. The palette deletes itself on close; the returned
  // pointer is for a caller that wants to look at it before then.
  CommandPalette* Show(const QString& initial_text);

  // Whether the palette offers address-space tags at all.
  bool has_tag_search() const { return tag_search_index_ != nullptr; }

 private:
  const AnyExecutor executor_;
  const CommandManager& command_manager_;
  const std::function<CommandHandler*(unsigned command_id)> resolve_command_;
  const std::function<void(const scada::NodeId& node_id,
                           const std::u16string& title)>
      open_tag_;
  QWidget& parent_;

  std::unique_ptr<TagSearchIndex> tag_search_index_;
  boost::signals2::scoped_connection session_state_connection_;
};
