#include "events/event_menu_model.h"

#include "aui/translation.h"
#include "controller/command_handler.h"
#include "resources/common_resources.h"

bool EventMenuModel::Delegate::IsCommandIdChecked(int command_id) const {
  auto* handler = commands_.GetCommandHandler(command_id);
  return handler && handler->IsCommandChecked(command_id);
}

bool EventMenuModel::Delegate::IsCommandIdEnabled(int command_id) const {
  auto* handler = commands_.GetCommandHandler(command_id);
  return handler && handler->IsCommandEnabled(command_id);
}

void EventMenuModel::Delegate::ExecuteCommand(int command_id) {
  if (auto* handler = commands_.GetCommandHandler(command_id))
    handler->ExecuteCommand(command_id);
}

EventMenuModel::EventMenuModel(CommandHandler& commands)
    : delegate_{commands}, severity_menu_{&delegate_}, model_{&delegate_} {
  // Minimum-severity filter, mutually exclusive: "All" (threshold 0) vs a
  // custom threshold. Both are checkable so the active one shows a check mark.
  severity_menu_.AddCheckItem(ID_SEVERITY_ALL, Translate("All"));
  severity_menu_.AddCheckItem(ID_SEVERITY_CUSTOM, Translate("Custom..."));

  model_.AddItem(ID_ACKNOWLEDGE_CURRENT, Translate("Acknowledge"));
  model_.AddItem(ID_ACKNOWLEDGE_ALL, Translate("Acknowledge All"));
  model_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  model_.AddCheckItem(ID_UNACKNOWLEDGED_ONLY, Translate("Unacknowledged Only"));
  model_.AddSubMenu(0, Translate("Severity"), &severity_menu_);
}

EventMenuModel::~EventMenuModel() = default;
