#include "client_utils_qt.h"

#include "ui/base/models/menu_model.h"

#include <QMenu>

void BuildMenu(QMenu& menu, ui::MenuModel& model) {
  model.MenuWillShow();

  for (int i = 0; i < model.GetItemCount(); ++i) {
    auto item_type = model.GetTypeAt(i);
    switch (item_type) {
      case ui::MenuModel::TYPE_SEPARATOR:
        menu.addSeparator();
        break;

      case ui::MenuModel::TYPE_SUBMENU:
        if (auto* submenu_model = model.GetSubmenuModelAt(i)) {
          auto* submenu =
              menu.addMenu(QString::fromStdWString(model.GetLabelAt(i)));
          QObject::connect(submenu, &QMenu::aboutToShow,
                           [submenu, submenu_model] {
                             submenu->clear();
                             BuildMenu(*submenu, *submenu_model);
                           });
        }
        break;

      case ui::MenuModel::TYPE_INPLACE_MENU:
        if (auto* inplace_model = model.GetSubmenuModelAt(i))
          BuildMenu(menu, *inplace_model);
        break;

      default: {
        auto* action =
            menu.addAction(QString::fromStdWString(model.GetLabelAt(i)));
        action->setEnabled(model.IsEnabledAt(i));
        if (item_type == ui::MenuModel::TYPE_CHECK ||
            item_type == ui::MenuModel::TYPE_RADIO) {
          action->setCheckable(true);
          action->setChecked(model.IsItemCheckedAt(i));
        }
        QObject::connect(action, &QAction::triggered,
                         [&model, i] { model.ActivatedAt(i); });
        break;
      }
    }
  }
}
