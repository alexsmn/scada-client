#include "ui/qt/client_utils_qt.h"

#include "aui/models/menu_model.h"
#include "aui/qt/image_util.h"
#include "resources/common_resources.h"

#include <QApplication>
#include <QMenu>
#include <QPalette>
#include <QString>

#include <array>
#include <string_view>
#include <utility>

namespace {

// Maps an integer command/action resource id to the Lucide glyph that draws
// it (docs/client/ux/iconography.md §5.1). Must stay in lock-step with
// res/client.qrc and common_resources.h.
//
// NOTE: the table is keyed by the raw numeric id. common_resources.h reuses
// numeric values across unrelated symbols; the ids below (the ones actually
// used as an Action image_id today) are all distinct, but a future
// .image_id_ that happens to share a number with one of these would silently
// resolve to the wrong icon.
constexpr std::array<std::pair<unsigned, std::string_view>, 17> kIconResources{{
    {ID_GRAPH_VIEW, ":/icons/chart-spline.svg"},
    {ID_MODUS_VIEW, ":/icons/workflow.svg"},
    {ID_TABLE_VIEW, ":/icons/table.svg"},
    {IDB_SUMMARY, ":/icons/clipboard-list.svg"},
    {ID_EVENT_VIEW, ":/icons/triangle-alert.svg"},
    {IDB_OPEN_EVENTS, ":/icons/logs.svg"},
    {IDB_TIMED_DATA, ":/icons/file-clock.svg"},
    {IDB_RECORD_EDITOR, ":/icons/file-pen.svg"},
    {IDB_PRINTER, ":/icons/printer.svg"},
    {IDB_WRITE, ":/icons/zap.svg"},
    {IDB_WRITE_MANUAL, ":/icons/pencil-line.svg"},
    {IDB_UNLOCK, ":/icons/lock-open.svg"},
    {IDB_COPY, ":/icons/copy.svg"},
    {IDB_PASTE, ":/icons/clipboard-paste.svg"},
    {IDB_DELETE, ":/icons/trash-2.svg"},
    {IDB_ACKNOWLEDGE_ALL, ":/icons/check-check.svg"},
    {ID_APPLICATION, ":/icons/settings.svg"},
}};

}  // namespace

void BuildMenu(QMenu& menu,
               scada::aui::MenuModel& model,
               const std::unordered_set<int>* skip_command_ids) {
  model.MenuWillShow();

  for (int i = 0; i < model.GetItemCount(); ++i) {
    auto item_type = model.GetTypeAt(i);
    switch (item_type) {
      case scada::aui::MenuModel::TYPE_SEPARATOR:
        menu.addSeparator();
        break;

      case scada::aui::MenuModel::TYPE_SUBMENU:
        if (auto* submenu_model = model.GetSubmenuModelAt(i)) {
          auto* submenu =
              menu.addMenu(QString::fromStdU16String(model.GetLabelAt(i)));
          BuildMenu(*submenu, *submenu_model, skip_command_ids);
          QObject::connect(submenu, &QMenu::aboutToShow,
                           [submenu, submenu_model, skip_command_ids] {
                             submenu->clear();
                             BuildMenu(*submenu, *submenu_model,
                                       skip_command_ids);
                           });
        }
        break;

      case scada::aui::MenuModel::TYPE_INPLACE_MENU:
        if (auto* inplace_model = model.GetSubmenuModelAt(i))
          BuildMenu(menu, *inplace_model, skip_command_ids);
        break;

      default: {
        if (skip_command_ids &&
            skip_command_ids->contains(model.GetCommandIdAt(i))) {
          break;
        }
        auto* action =
            menu.addAction(QString::fromStdU16String(model.GetLabelAt(i)));
        action->setData(model.GetCommandIdAt(i));
        const bool enabled = model.IsEnabledAt(i);
        action->setEnabled(enabled);
        // A greyed entry explains itself on hover rather than leaving the
        // operator to guess whether the system is broken or the action simply
        // does not apply. Qt hides menu tooltips unless asked, and only shows
        // them for disabled items when the menu opts in.
        if (!enabled) {
          const std::u16string reason = model.GetDisabledReasonAt(i);
          if (!reason.empty()) {
            action->setToolTip(QString::fromStdU16String(reason));
            menu.setToolTipsVisible(true);
          }
        }
        if (item_type == scada::aui::MenuModel::TYPE_CHECK ||
            item_type == scada::aui::MenuModel::TYPE_RADIO) {
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

QPixmap LoadPixmap(unsigned resource_id, int size) {
  for (const auto& [id, path] : kIconResources) {
    if (id != resource_id)
      continue;
    // Tinted from the application palette, because the files carry
    // stroke="currentColor" and Qt's SVG renderer resolves that to black.
    // Rendered at the requested size times the device pixel ratio, so a
    // toolbar glyph is drawn rather than upscaled from a 16 px raster.
    //
    // The tint is taken when the icon is built, which is when the action is
    // created. A live theme switch therefore leaves already-built toolbar
    // actions on the old tint until they are rebuilt — menus rebuild
    // themselves on aboutToShow, toolbars do not. Acceptable while the theme
    // is settled at startup; if live switching becomes real, this wants a
    // QIconEngine that re-renders per palette.
    return LoadTintedGlyph(path, size,
                           QApplication::palette().color(QPalette::WindowText),
                           qApp ? qApp->devicePixelRatio() : 1.0)
        .pixmap(size, size);
  }
  return {};
}
