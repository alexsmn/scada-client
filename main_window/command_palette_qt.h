#pragma once

#include "main_window/command_match.h"

#include <QDialog>

#include <functional>
#include <vector>

class CommandManager;
class CommandHandler;
class QLineEdit;
class QListWidget;

// A Ctrl-K command palette: type to filter every registered command by title
// and run the selected one. Commands are executed through the supplied
// resolver (the main window's handler resolution), so only enabled commands
// actually run and the palette needs no direct knowledge of command contexts.
class CommandPalette : public QDialog {
  Q_OBJECT

 public:
  // Resolves the handler that would run `command_id`, or nullptr if none. The
  // palette checks IsCommandEnabled and calls ExecuteCommand on it.
  using HandlerResolver = std::function<CommandHandler*(unsigned command_id)>;

  CommandPalette(QWidget* parent,
                 const CommandManager& command_manager,
                 HandlerResolver resolver);
  ~CommandPalette() override;

 protected:
  // QObject
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  void Refilter();
  void ActivateCurrent();
  void MoveSelection(int delta);

  std::vector<CommandEntry> entries_;
  HandlerResolver resolver_;

  QLineEdit* filter_ = nullptr;
  QListWidget* list_ = nullptr;
};
