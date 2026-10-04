#include "main_window/menu_bar_qt.h"

#include "aui/models/simple_menu_model.h"
#include "aui/test/app_environment.h"

#include <QAction>
#include <QMenu>
#include <QMenuBar>

#include <gmock/gmock.h>

#include <vector>

using namespace testing;

namespace {

// The items of `menu`, by text, without separators or the macOS "Loading..."
// placeholder a menu carries until its first rebuild.
std::vector<QString> ItemTexts(const QMenu& menu) {
  std::vector<QString> texts;
  for (const QAction* action : menu.actions()) {
    if (!action->isSeparator() &&
        action->text() != QStringLiteral("Loading..."))
      texts.push_back(action->text());
  }
  return texts;
}

class MenuBarTest : public Test {
 protected:
  MenuBarTest() {
    file_.AddItem(1, u"Open");
    view_.AddItem(2, u"Table");
    root_.AddSubMenu(10, u"File", &file_);
    root_.AddSubMenu(11, u"View", &view_);
  }

  QMenu& TopLevelMenu(int index) {
    QMenu* menu = menu_bar_.actions().at(index)->menu();
    EXPECT_NE(menu, nullptr);
    return *menu;
  }

  // Opens `menu` the way Qt does. aboutToShow is a signal and cannot be
  // emitted from outside QMenu; invoking it through the meta-object is the
  // supported equivalent.
  static void Show(QMenu& menu) {
    QMetaObject::invokeMethod(&menu, "aboutToShow");
  }

  // Per-test QApplication; see ActivityBarTest for why it is never static.
  AppEnvironment app_env_;
  scada::aui::SimpleMenuModel file_{nullptr};
  scada::aui::SimpleMenuModel view_{nullptr};
  scada::aui::SimpleMenuModel root_{nullptr};
  QMenuBar menu_bar_;
};

TEST_F(MenuBarTest, OneTopLevelMenuPerSubmenuInModelOrder) {
  PopulateMenuBar(menu_bar_, root_);

  ASSERT_THAT(menu_bar_.actions(), SizeIs(2));
  EXPECT_EQ(menu_bar_.actions()[0]->text(), QStringLiteral("File"));
  EXPECT_EQ(menu_bar_.actions()[1]->text(), QStringLiteral("View"));
}

// What a menu shows is re-read from its model every time it opens, so a model
// that changed since the last opening is never shown stale.
TEST_F(MenuBarTest, OpeningAMenuRebuildsItFromItsModel) {
  PopulateMenuBar(menu_bar_, root_);

  Show(TopLevelMenu(0));
  EXPECT_THAT(ItemTexts(TopLevelMenu(0)), ElementsAre(QStringLiteral("Open")));

  file_.AddItem(3, u"Save");
  Show(TopLevelMenu(0));
  EXPECT_THAT(ItemTexts(TopLevelMenu(0)),
              ElementsAre(QStringLiteral("Open"), QStringLiteral("Save")));
}

TEST_F(MenuBarTest, RebuildFillsEveryMenuWithoutOpeningIt) {
  PopulateMenuBar(menu_bar_, root_);

  view_.AddItem(4, u"Graph");
  RebuildMenuBar(menu_bar_, root_);

  EXPECT_THAT(ItemTexts(TopLevelMenu(0)), ElementsAre(QStringLiteral("Open")));
  EXPECT_THAT(ItemTexts(TopLevelMenu(1)),
              ElementsAre(QStringLiteral("Table"), QStringLiteral("Graph")));
}

}  // namespace
