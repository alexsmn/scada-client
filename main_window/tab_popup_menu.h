#pragma once

#include "aui/models/simple_menu_model.h"
#include "aui/translation.h"
#include "main_window/simple_menu_command_handler.h"
#include "resources/common_resources.h"

class TabPopupMenu : public scada::aui::SimpleMenuModel {
 public:
  explicit TabPopupMenu(CommandHandler& commands)
      : scada::aui::SimpleMenuModel{&handler_}, handler_{commands} {
    AddItem(ID_VIEW_ADD_TO_FAVOURITES, Translate("To Favourites"));
    AddItem(ID_VIEW_CHANGE_TITLE, Translate("Rename"));
    AddSeparator(scada::aui::NORMAL_SEPARATOR);
    AddItem(ID_VIEW_CLOSE, Translate("Close"));
  }

 private:
  SimpleMenuCommandHandler handler_;
};
