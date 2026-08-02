#include "modules/table/table_menu_model.h"

#include "aui/translation.h"
#include "controller/command_handler.h"
#include "resources/common_resources.h"

bool TableMenuModel::Delegate::IsCommandIdChecked(int command_id) const {
  auto* handler = commands_.GetCommandHandler(command_id);
  return handler && handler->IsCommandChecked(command_id);
}

bool TableMenuModel::Delegate::IsCommandIdEnabled(int command_id) const {
  auto* handler = commands_.GetCommandHandler(command_id);
  return handler && handler->IsCommandEnabled(command_id);
}

void TableMenuModel::Delegate::ExecuteCommand(int command_id) {
  if (auto* handler = commands_.GetCommandHandler(command_id))
    handler->ExecuteCommand(command_id);
}

TableMenuModel::TableMenuModel(CommandHandler& commands)
    : delegate_{commands}, sort_menu_{&delegate_}, model_{&delegate_} {
  // Sort-by submenu: the row order key. Both are checkable so the active key
  // shows a check mark (delegated to the view's command registry).
  sort_menu_.AddCheckItem(ID_SORT_NAME, Translate("Name"));
  sort_menu_.AddCheckItem(ID_SORT_CHANNEL, Translate("Channel"));

  model_.AddItem(ID_RENAME, Translate("Rename"));
  model_.AddItem(ID_MOVE_UP, Translate("Move Up"));
  model_.AddItem(ID_MOVE_DOWN, Translate("Move Down"));
  model_.AddItem(ID_DELETE, Translate("Delete Row"));
  model_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  model_.AddSubMenu(0, Translate("Sort"), &sort_menu_);
}

TableMenuModel::~TableMenuModel() = default;
