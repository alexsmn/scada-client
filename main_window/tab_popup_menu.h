#pragma once

#include "aui/models/simple_menu_model.h"
#include "resources/common_resources.h"
#include "main_window/simple_menu_command_handler.h"

class TabPopupMenu : public aui::SimpleMenuModel {
 public:
  explicit TabPopupMenu(CommandHandler& commands)
      : aui::SimpleMenuModel{&handler_}, handler_{commands} {
    AddItem(ID_VIEW_ADD_TO_FAVOURITES, u"\u0412 \u0438\u0437\u0431\u0440\u0430\u043d\u043d\u043e\u0435");
    AddItem(ID_VIEW_CHANGE_TITLE, u"\u041f\u0435\u0440\u0435\u0438\u043c\u0435\u043d\u043e\u0432\u0430\u0442\u044c");
    AddSeparator(aui::NORMAL_SEPARATOR);
    AddItem(ID_VIEW_CLOSE, u"\u0417\u0430\u043a\u0440\u044b\u0442\u044c");
  }

 private:
  SimpleMenuCommandHandler handler_;
};
