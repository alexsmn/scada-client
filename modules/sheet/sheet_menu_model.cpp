#include "modules/sheet/sheet_menu_model.h"

#include "aui/translation.h"
#include "controller/command_handler.h"
#include "resources/common_resources.h"

bool SheetMenuModel::Delegate::IsCommandIdChecked(int command_id) const {
  auto* handler = commands_.GetCommandHandler(command_id);
  return handler && handler->IsCommandChecked(command_id);
}

bool SheetMenuModel::Delegate::IsCommandIdEnabled(int command_id) const {
  auto* handler = commands_.GetCommandHandler(command_id);
  return handler && handler->IsCommandEnabled(command_id);
}

void SheetMenuModel::Delegate::ExecuteCommand(int command_id) {
  if (auto* handler = commands_.GetCommandHandler(command_id))
    handler->ExecuteCommand(command_id);
}

SheetMenuModel::SheetMenuModel(CommandHandler& commands)
    : delegate_{commands}, model_{&delegate_} {
  model_.AddItem(ID_GRAPH_COLOR, Translate("Color..."));
}

SheetMenuModel::~SheetMenuModel() = default;
