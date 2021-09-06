#pragma once

#include "ui/base/models/menu_model.h"

#pragma warning(push)
#pragma warning(disable : 4251 4275)
#include <wt/WWidget.h>
#pragma warning(pop)

namespace ui {
class MenuModel;
}

struct MainMenuControllerContext {
  std::unique_ptr<ui::MenuModel> main_menu_model_;
};

class MainMenuController : private MainMenuControllerContext {
 public:
  explicit MainMenuController(MainMenuControllerContext&& context);

  std::unique_ptr<Wt::WWidget> CreateWidget();
};