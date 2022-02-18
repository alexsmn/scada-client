#include "client_utils_qt.h"

#include "base/string_piece_util.h"
#include "base/strings/utf_string_conversions.h"
#include "translation.h"
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
              menu.addMenu(QString::fromStdU16String(model.GetLabelAt(i)));
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
            menu.addAction(QString::fromStdU16String(model.GetLabelAt(i)));
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

std::u16string Translate(std::string_view text) {
  return QObject::tr(std::string(text).c_str()).toStdU16String();
}

std::u16string FormatHostName(std::string_view host_name) {
  if (host_name.empty()) {
    return QObject::tr("Local").toStdU16String();
  } else {
    return base::UTF8ToUTF16(AsStringPiece(host_name));
  }
}

QPixmap LoadPixmap(unsigned resource_id) {
  HRSRC hres = FindResource(NULL, MAKEINTRESOURCE(resource_id), L"PNG");
  DWORD size = SizeofResource(NULL, hres);

  HGLOBAL resource = LoadResource(NULL, hres);

  LPVOID resource_data = LockResource(resource);

  QPixmap pixmap;
  pixmap.loadFromData(static_cast<const uchar*>(resource_data), size);
  return pixmap;
}
