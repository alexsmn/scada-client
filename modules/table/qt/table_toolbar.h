#pragma once

#include <QWidget>

#include <functional>
#include <vector>

class CommandHandler;
class QToolButton;

// Inputs for the Table workspace toolbar (Qt) — the reshell's discoverable
// surfacing of the grid's row commands (table-watch.html), complementing the
// right-click context menu (`TableMenuModel`).
struct TableToolbarContext {
  // Resolves a command id to the handler currently accepting it — the view's
  // own registry first, then the shell surface via
  // `ControllerDelegate::ResolveViewCommand`. A null result means the command
  // is unavailable in this session and its control hides itself.
  std::function<CommandHandler*(unsigned)> resolve_command;
  // Begins entry of a new signal/expression row (opens the editor on the
  // grid's trailing "Enter expression" row).
  std::function<void()> on_add_signal;
};

// The Table toolbar: add-signal, row commands (delete/move), sort keys, and
// the selection/export commands (to-graph, CSV, print), each executing through
// `resolve_command` and tracking its availability/enablement via Refresh().
class TableToolbar : public QWidget {
 public:
  explicit TableToolbar(TableToolbarContext context, QWidget* parent = nullptr);
  ~TableToolbar() override;

  // Re-reads each command's availability (button shown), enablement and
  // checked state (sort keys) from `resolve_command`. The host calls this on
  // selection and model changes; it also runs after every button press.
  void Refresh();

 protected:
  // The shell commands resolve against the *active* view, so the state read
  // at construction (before this view is activated) can be stale — re-read
  // whenever the bar becomes visible.
  virtual void showEvent(QShowEvent* event) override;

 private:
  // A button bound to a shell/view command id.
  struct CommandButton {
    unsigned command_id = 0;
    QToolButton* button = nullptr;
    bool checkable = false;
  };

  QToolButton* AddCommandButton(unsigned command_id,
                                const QString& label,
                                bool checkable = false);
  void ExecuteCommand(unsigned command_id);

  TableToolbarContext context_;
  std::vector<CommandButton> command_buttons_;
};

// Builds the table toolbar (docs/client/ux/backlog.md 2.8). Ownership
// transfers to the caller.
TableToolbar* MakeTableToolbar(TableToolbarContext context);
