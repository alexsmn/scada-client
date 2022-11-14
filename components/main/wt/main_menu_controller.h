#pragma once

#include "controls/models/menu_model.h"

#pragma warning(push)
#pragma warning(disable : 4251 4275)
#include <wt/WWidget.h>
#pragma warning(pop)

namespace aui {
class MenuModel;
}

struct MainMenuControllerContext {
  std::unique_ptr<aui::MenuModel> main_menu_model_;
};

class MainMenuController : private MainMenuControllerContext {
 public:
  explicit MainMenuController(MainMenuControllerContext&& context);

  std::unique_ptr<Wt::WWidget> CreateWidget();
};
