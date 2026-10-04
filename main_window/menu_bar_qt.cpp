#include "main_window/menu_bar_qt.h"

#include "aui/models/menu_model.h"
#include "base/check.h"
#include "ui/qt/client_utils_qt.h"

#include <QCoreApplication>
#include <QMenu>
#include <QMenuBar>
#include <QString>

void PopulateMenuBar(QMenuBar& menu_bar, scada::aui::MenuModel& model) {
  for (int i = 0; i < model.GetItemCount(); ++i) {
    const std::u16string label = model.GetLabelAt(i);
    auto* submenu = menu_bar.addMenu(QString::fromStdU16String(label));
    auto* submenu_model = model.GetSubmenuModelAt(i);
    scada::base::Check(submenu_model);
    QObject::connect(submenu, &QMenu::aboutToShow, &menu_bar,
                     [submenu, submenu_model] {
                       submenu->clear();
                       BuildMenu(*submenu, *submenu_model);
                     });
#ifdef __APPLE__
    // The native menu bar shows a menu's items as they were before
    // aboutToShow, so an empty menu would read as nothing to choose until the
    // first rebuild. The source keeps the MainWindow context it has always had
    // in client_ru.ts.
    auto* loading_action = submenu->addAction(
        QCoreApplication::translate("MainWindow", "Loading..."));
    loading_action->setEnabled(false);
#endif
  }
}

void RebuildMenuBar(QMenuBar& menu_bar, scada::aui::MenuModel& model) {
  const auto top_level_actions = menu_bar.actions();
  // A menu appended outside the model is allowed, so the bar may hold more.
  scada::base::Check(top_level_actions.size() >= model.GetItemCount());

  for (int i = 0; i < model.GetItemCount(); ++i) {
    auto* submenu = top_level_actions[i]->menu();
    auto* submenu_model = model.GetSubmenuModelAt(i);
    scada::base::Check(submenu);
    scada::base::Check(submenu_model);
    submenu->clear();
    BuildMenu(*submenu, *submenu_model);
  }
}
