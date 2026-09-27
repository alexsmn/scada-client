#include "main_window/command_actions_qt.h"

#include "aui/qt/key_codes.h"
#include "controller/command_handler.h"
#include "controller/command_manager.h"
#include "ui/qt/client_utils_qt.h"

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QObject>

#include <utility>

CommandActions::CommandActions(const CommandManager& command_manager,
                               ActionManager& action_manager,
                               HandlerResolver resolve_handler,
                               QObject& parent)
    : action_manager_{action_manager},
      resolve_handler_{std::move(resolve_handler)} {
  for (const CommandDescriptor* command : command_manager.commands()) {
    if (!command->show_in_toolbar)
      continue;

    // A command whose category collapses behind a group button yields to the
    // expanded ones when a surface runs out of room.
    const bool collapsible =
        !CanExpandCommandCategory(command->category, CommandSurface::kToolbar);
    auto* action = new QAction(
        QString::fromStdU16String(command->GetShortTitle()), &parent);
    action->setPriority(collapsible ? QAction::LowPriority
                                    : QAction::NormalPriority);
    action->setVisible(false);
    if (command->image_id != 0)
      action->setIcon(QIcon(LoadPixmap(command->image_id)));
    action->setCheckable(command->checkable());
    if (command->shortcut.has_value()) {
      action->setShortcut(scada::aui::ToQKeySequence(
          command->shortcut->modifiers(), command->shortcut->key_code()));
    }

    // Captures the resolver by value rather than `this`: the action belongs to
    // `parent` and can outlive this registry.
    const unsigned command_id = command->command_id;
    QObject::connect(action, &QAction::triggered,
                     [resolve = resolve_handler_, command_id](bool) {
                       CommandHandler* handler = resolve(command_id);
                       if (handler && handler->IsCommandEnabled(command_id))
                         handler->ExecuteCommand(command_id);
                     });

    actions_.emplace(command_id, action);
    command_ids_.emplace(action, command_id);
  }

  action_changed_connection_ = action_manager_.Subscribe(
      [this](Action& action, ActionChangeMask change_mask) {
        OnActionChanged(action, change_mask);
      });
}

CommandActions::~CommandActions() = default;

QAction* CommandActions::Find(unsigned command_id) const {
  auto i = actions_.find(command_id);
  return i == actions_.end() ? nullptr : i->second;
}

void CommandActions::UpdateAll() {
  for (const auto& [command_id, action] : actions_)
    Update(*action, command_id, ActionChangeMask::AllButTitle);
}

void CommandActions::UpdateMenu(QMenu& menu) {
  for (QAction* action : menu.actions()) {
    auto i = command_ids_.find(action);
    if (i != command_ids_.end())
      Update(*action, i->second, ActionChangeMask::All);
  }
}

void CommandActions::Update(QAction& action,
                            unsigned command_id,
                            ActionChangeMask change_mask) {
  if (static_cast<unsigned>(change_mask) &
      static_cast<unsigned>(ActionChangeMask::Title)) {
    if (const Action* source = action_manager_.FindAction(command_id))
      action.setText(QString::fromStdU16String(source->GetTitle()));
  }

  const CommandHandler* handler = resolve_handler_(command_id);
  action.setVisible(handler != nullptr);
  if (handler) {
    const bool enabled = handler->IsCommandEnabled(command_id);
    action.setEnabled(enabled);
    if (enabled)
      action.setChecked(handler->IsCommandChecked(command_id));
  }
}

void CommandActions::OnActionChanged(Action& action,
                                     ActionChangeMask change_mask) {
  auto i = actions_.find(action.command_id());
  if (i != actions_.end())
    Update(*i->second, i->first, change_mask);
}
