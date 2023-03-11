#include "components/main/wt/main_menu_controller.h"

#pragma warning(push)
#pragma warning(disable : 4251 4275)
#include <wt/WMenu.h>
#include <wt/WNavigationBar.h>
#include <wt/WPopupMenu.h>
#pragma warning(pop)

namespace {

std::unique_ptr<Wt::WPopupMenu> CreatePopupMenu(aui::MenuModel& model);

class TestPopupMenu : public Wt::WPopupMenu {
 public:
  explicit TestPopupMenu(aui::MenuModel& model) : model_{model} {}

  virtual void render(Wt::WFlags<Wt::RenderFlag> flags) override {
    while (count() != 0)
      removeItem(itemAt(count() - 1));

    model_.MenuWillShow();

    for (int i = 0; i < model_.GetItemCount(); ++i) {
      if (!model_.IsVisibleAt(i))
        continue;

      auto type = model_.GetTypeAt(i);
      switch (type) {
        default: {
          auto* item = addItem(model_.GetLabelAt(i));
          item->setCheckable(type == aui::MenuModel::ItemType::TYPE_CHECK);
          item->setDisabled(!model_.IsEnabledAt(i));
          item->setChecked(model_.IsItemCheckedAt(i));
          item->triggered().connect(
              [index = i, &model = model_] { model.ActivatedAt(index); });
          break;
        }

        case aui::MenuModel::ItemType::TYPE_SUBMENU: {
          addMenu(model_.GetLabelAt(i),
                  CreatePopupMenu(*model_.GetSubmenuModelAt(i)));
          break;
        }

        case aui::MenuModel::ItemType::TYPE_SEPARATOR:
          addSeparator();
          break;
      }
    }

    Wt::WPopupMenu::render(flags);
  }

 private:
  aui::MenuModel& model_;
};

std::unique_ptr<Wt::WPopupMenu> CreatePopupMenu(aui::MenuModel& model) {
  auto popup = std::make_unique<TestPopupMenu>(model);

  /*for (int i = 0; i < model.GetItemCount(); ++i) {
    auto type = model.GetTypeAt(i);
    switch (type) {
      default: {
        auto* item = popup->addItem(model.GetLabelAt(i));
        item->setCheckable(type == aui::MenuModel::ItemType::TYPE_CHECK);
        break;
      }

      case aui::MenuModel::ItemType::TYPE_SEPARATOR:
        popup->addSeparator();
        break;
    }
  }*/

  return popup;
}

}  // namespace

MainMenuController::MainMenuController(MainMenuControllerContext&& context)
    : MainMenuControllerContext{std::move(context)} {}

std::unique_ptr<Wt::WWidget> MainMenuController::CreateWidget() {
  auto navigation = std::make_unique<Wt::WNavigationBar>();
  navigation->setTitle(L"Телеконтроль", "http://telecontrol.ru");
  navigation->setResponsive(true);

  auto* menu = navigation->addMenu(std::make_unique<Wt::WMenu>());
  for (int i = 0; i < main_menu_model_->GetItemCount(); ++i) {
    auto label = main_menu_model_->GetLabelAt(i);
    auto* submenu_model = main_menu_model_->GetSubmenuModelAt(i);
    assert(submenu_model);
    auto item = std::make_unique<Wt::WMenuItem>(std::move(label));
    item->setMenu(CreatePopupMenu(*submenu_model));
    menu->addItem(std::move(item));
  }

  return navigation;
}