#include "modules/watch/watch_menu_model.h"

#include "aui/translation.h"
#include "controller/command_handler.h"
#include "resources/common_resources.h"

bool WatchMenuModel::Delegate::IsCommandIdChecked(int command_id) const {
  auto* handler = commands_.GetCommandHandler(command_id);
  return handler && handler->IsCommandChecked(command_id);
}

bool WatchMenuModel::Delegate::IsCommandIdEnabled(int command_id) const {
  auto* handler = commands_.GetCommandHandler(command_id);
  return handler && handler->IsCommandEnabled(command_id);
}

void WatchMenuModel::Delegate::ExecuteCommand(int command_id) {
  if (auto* handler = commands_.GetCommandHandler(command_id))
    handler->ExecuteCommand(command_id);
}

WatchMenuModel::WatchMenuModel(CommandHandler& commands)
    : delegate_{commands}, model_{&delegate_} {
  // Pause is a checkable toggle; its check mark tracks the view's paused state
  // through the command registry.
  model_.AddCheckItem(ID_PAUSE, Translate("Pause"));
  model_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  model_.AddItem(ID_SAVE_AS, Translate("Save As..."));
  model_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  model_.AddItem(ID_CLEAR_ALL, Translate("Clear"));
}

WatchMenuModel::~WatchMenuModel() = default;
