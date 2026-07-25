#include "modules/node_table/node_table_menu_model.h"

#include "aui/translation.h"
#include "controller/command_handler.h"
#include "resources/common_resources.h"

bool NodeTableMenuModel::Delegate::IsCommandIdChecked(int command_id) const {
  auto* handler = commands_.GetCommandHandler(command_id);
  return handler && handler->IsCommandChecked(command_id);
}

bool NodeTableMenuModel::Delegate::IsCommandIdEnabled(int command_id) const {
  auto* handler = commands_.GetCommandHandler(command_id);
  return handler && handler->IsCommandEnabled(command_id);
}

void NodeTableMenuModel::Delegate::ExecuteCommand(int command_id) {
  if (auto* handler = commands_.GetCommandHandler(command_id))
    handler->ExecuteCommand(command_id);
}

NodeTableMenuModel::NodeTableMenuModel(CommandHandler& commands)
    : delegate_{commands}, sort_menu_{&delegate_}, model_{&delegate_} {
  // Sort-by submenu: the row order key. All three are checkable so the active
  // key shows a check mark (delegated to the controller's command registry).
  sort_menu_.AddCheckItem(ID_SORT_NONE, Translate("None"));
  sort_menu_.AddCheckItem(ID_SORT_ALIAS, Translate("Alias"));
  sort_menu_.AddCheckItem(ID_SORT_CHANNEL, Translate("Channel"));

  model_.AddItem(ID_RENAME, Translate("Rename"));
  model_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  model_.AddSubMenu(0, Translate("Sort"), &sort_menu_);
}

NodeTableMenuModel::~NodeTableMenuModel() = default;
