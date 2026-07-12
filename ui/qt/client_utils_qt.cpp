#include "ui/qt/client_utils_qt.h"

#include "aui/models/menu_model.h"
#include "resources/common_resources.h"

#include <QMenu>
#include <QString>

#include <array>
#include <string_view>
#include <utility>

namespace {

// Maps an integer command/action resource id to the Qt resource path of its
// icon (packaged in res/client.qrc under the "/res" prefix). This is the
// cross-platform replacement for the former Win32 FindResource("PNG") lookup,
// and must stay in lock-step with client.qrc and common_resources.h.
//
// NOTE: the table is keyed by the raw numeric id. common_resources.h reuses
// numeric values across unrelated symbols; the ids below (the ones actually
// used as an Action image_id today) are all distinct, but a future
// .image_id_ that happens to share a number with one of these would silently
// resolve to the wrong icon.
constexpr std::array<std::pair<unsigned, std::string_view>, 17> kIconResources{{
    {ID_GRAPH_VIEW, ":/res/chart_curve.png"},
    {ID_MODUS_VIEW, ":/res/display.png"},
    {ID_TABLE_VIEW, ":/res/table.png"},
    {IDB_SUMMARY, ":/res/summary.png"},
    {ID_EVENT_VIEW, ":/res/event_view.png"},
    {IDB_OPEN_EVENTS, ":/res/events3.png"},
    {IDB_TIMED_DATA, ":/res/doc_table.png"},
    {IDB_RECORD_EDITOR, ":/res/record_editor.png"},
    {IDB_PRINTER, ":/res/printer.png"},
    {IDB_WRITE, ":/res/execute.png"},
    {IDB_WRITE_MANUAL, ":/res/write_manual.png"},
    {IDB_UNLOCK, ":/res/unlock.png"},
    {IDB_COPY, ":/res/copy.png"},
    {IDB_PASTE, ":/res/paste.png"},
    {IDB_DELETE, ":/res/delete.png"},
    {IDB_ACKNOWLEDGE_ALL, ":/res/acknowledge_all.png"},
    {ID_APPLICATION, ":/res/settings/settings64-32bit.png"},
}};

}  // namespace

void BuildMenu(QMenu& menu, aui::MenuModel& model) {
  model.MenuWillShow();

  for (int i = 0; i < model.GetItemCount(); ++i) {
    auto item_type = model.GetTypeAt(i);
    switch (item_type) {
      case aui::MenuModel::TYPE_SEPARATOR:
        menu.addSeparator();
        break;

      case aui::MenuModel::TYPE_SUBMENU:
        if (auto* submenu_model = model.GetSubmenuModelAt(i)) {
          auto* submenu =
              menu.addMenu(QString::fromStdU16String(model.GetLabelAt(i)));
          BuildMenu(*submenu, *submenu_model);
          QObject::connect(submenu, &QMenu::aboutToShow,
                           [submenu, submenu_model] {
                             submenu->clear();
                             BuildMenu(*submenu, *submenu_model);
                           });
        }
        break;

      case aui::MenuModel::TYPE_INPLACE_MENU:
        if (auto* inplace_model = model.GetSubmenuModelAt(i))
          BuildMenu(menu, *inplace_model);
        break;

      default: {
        auto* action =
            menu.addAction(QString::fromStdU16String(model.GetLabelAt(i)));
        action->setData(model.GetCommandIdAt(i));
        action->setEnabled(model.IsEnabledAt(i));
        if (item_type == aui::MenuModel::TYPE_CHECK ||
            item_type == aui::MenuModel::TYPE_RADIO) {
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

QPixmap LoadPixmap(unsigned resource_id) {
  for (const auto& [id, path] : kIconResources) {
    if (id == resource_id)
      return QPixmap(QString::fromUtf8(path.data(), path.size()));
  }
  return {};
}
