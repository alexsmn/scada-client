#include "modules/favorites/favourites_view_menu_model.h"

#include "aui/translation.h"
#include "controller/command_handler.h"
#include "resources/common_resources.h"

bool FavouritesViewMenuModel::Delegate::IsCommandIdChecked(
    int command_id) const {
  auto* handler = commands_.GetCommandHandler(command_id);
  return handler && handler->IsCommandChecked(command_id);
}

bool FavouritesViewMenuModel::Delegate::IsCommandIdEnabled(
    int command_id) const {
  auto* handler = commands_.GetCommandHandler(command_id);
  return handler && handler->IsCommandEnabled(command_id);
}

void FavouritesViewMenuModel::Delegate::ExecuteCommand(int command_id) {
  if (auto* handler = commands_.GetCommandHandler(command_id))
    handler->ExecuteCommand(command_id);
}

FavouritesViewMenuModel::FavouritesViewMenuModel(CommandHandler& commands)
    : delegate_{commands}, model_{&delegate_} {
  model_.AddItem(ID_OPEN, Translate("Open"));
  model_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  model_.AddItem(ID_FAVOURITES_ADD_URL, Translate("Add Web Page..."));
  model_.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  model_.AddItem(ID_RENAME, Translate("Rename"));
  model_.AddItem(ID_DELETE, Translate("Delete"));
}

FavouritesViewMenuModel::~FavouritesViewMenuModel() = default;
