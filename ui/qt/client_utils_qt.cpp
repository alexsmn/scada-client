#include "ui/qt/client_utils_qt.h"

#include "aui/models/menu_model.h"
#include "aui/qt/image_util.h"
#include "resources/common_resources.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QMenu>
#include <QPalette>
#include <QString>

#include <array>
#include <map>
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

// Marks the exclusive QActionGroups BuildMenu creates for radio items, so a
// rebuild discards its own predecessors and leaves alone any group a caller
// parented to the same menu.
QString RadioGroupObjectName() {
  return QStringLiteral("scada.BuildMenu.radioGroup");
}

// The exclusive groups created while building one QMenu, keyed by the model
// that owns the radio item and that model's own group id. The model is part of
// the key because an in-place submenu is merged into the *same* QMenu, and its
// group 0 is a different group from the parent's group 0.
using RadioGroups =
    std::map<std::pair<const scada::aui::MenuModel*, int>, QActionGroup*>;

QActionGroup& RadioGroupFor(QMenu& menu,
                            const scada::aui::MenuModel& model,
                            int group_id,
                            RadioGroups& radio_groups) {
  QActionGroup*& group = radio_groups[{&model, group_id}];
  if (!group) {
    // Parented to the menu so it outlives this call and dies with it.
    // Exclusive by default, which is the property Qt keys the radio indicator
    // off — see the call site.
    group = new QActionGroup{&menu};
    group->setObjectName(RadioGroupObjectName());
  }
  return *group;
}

// Drops the groups left behind by the previous build of |menu|. BuildMenu is
// re-run on every aboutToShow and QMenu::clear() deletes the actions but not
// the groups, so without this they accumulate for the life of the menu.
// Actions are unlinked first: ~QActionGroup does not clear the back-pointer
// its actions hold, and ~QAction dereferences it.
void DiscardRadioGroups(QMenu& menu) {
  const QString name = RadioGroupObjectName();
  for (QActionGroup* group :
       menu.findChildren<QActionGroup*>(Qt::FindDirectChildrenOnly)) {
    if (group->objectName() != name)
      continue;
    for (QAction* action : group->actions())
      group->removeAction(action);
    delete group;
  }
}

void AppendMenuItems(QMenu& menu,
                     scada::aui::MenuModel& model,
                     const std::unordered_set<int>* skip_command_ids,
                     RadioGroups& radio_groups);

}  // namespace

void BuildMenu(QMenu& menu,
               scada::aui::MenuModel& model,
               const std::unordered_set<int>* skip_command_ids) {
  DiscardRadioGroups(menu);

  RadioGroups radio_groups;
  AppendMenuItems(menu, model, skip_command_ids, radio_groups);
}

namespace {

// Appends |model|'s items to |menu|. Split from BuildMenu so an in-place
// submenu, which lands in the same QMenu, can share the caller's radio groups
// rather than restarting them (and rather than re-running the per-menu
// cleanup, which would delete the groups just created).
void AppendMenuItems(QMenu& menu,
                     scada::aui::MenuModel& model,
                     const std::unordered_set<int>* skip_command_ids,
                     RadioGroups& radio_groups) {
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
        if (auto* inplace_model = model.GetSubmenuModelAt(i)) {
          AppendMenuItems(menu, *inplace_model, skip_command_ids, radio_groups);
        }
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
        // Checkable alone gets a check box, which reads as "an independent
        // toggle". Qt only draws the radio indicator that means "pick one" for
        // an action belonging to an exclusive QActionGroup (QMenu sets
        // QStyleOptionMenuItem::Exclusive off actionGroup()->isExclusive()),
        // so a radio item has to join one.
        if (item_type == scada::aui::MenuModel::TYPE_RADIO) {
          action->setActionGroup(
              &RadioGroupFor(menu, model, model.GetGroupIdAt(i), radio_groups));
        }
        QObject::connect(action, &QAction::triggered,
                         [&model, i] { model.ActivatedAt(i); });
        break;
      }
    }
  }
}

}  // namespace

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
