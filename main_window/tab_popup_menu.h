#pragma once

#include "aui/models/simple_menu_model.h"
#include "resources/common_resources.h"
#include "main_window/simple_menu_command_handler.h"

class TabPopupMenu : public aui::SimpleMenuModel {
 public:
  explicit TabPopupMenu(CommandHandler& commands)
      : aui::SimpleMenuModel{&handler_}, handler_{commands} {
    AddItem(ID_VIEW_ADD_TO_FAVOURITES, u"В избранное");
    AddItem(ID_VIEW_CHANGE_TITLE, u"Переименовать");
    AddSeparator(aui::NORMAL_SEPARATOR);
    AddItem(ID_VIEW_CLOSE, u"Закрыть");
  }

 private:
  SimpleMenuCommandHandler handler_;
};
