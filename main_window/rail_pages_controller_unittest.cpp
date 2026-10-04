#include "main_window/rail_pages_controller.h"

#include "aui/dialog_service_mock.h"
#include "aui/test/app_environment.h"
#include "aui/translation.h"
#include "base/test/test_executor.h"
#include "main_window/activity_bar_qt.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/main_window_mock.h"
#include "main_window/page_icons.h"
#include "profile/profile.h"
#include "resources/common_resources.h"

#include <QAction>
#include <QMenu>
#include <QToolButton>
#include <QWidget>

#include <gmock/gmock.h>

#include <memory>
#include <vector>

using namespace testing;

namespace {

class RailPagesControllerTest : public Test {
 protected:
  RailPagesControllerTest() {
    first_id_ = AddPage(u"First").id;
    second_id_ = AddPage(u"Second").id;
    third_id_ = AddPage(u"Third").id;
    EXPECT_CALL(main_window_, GetCurrentPage())
        .WillRepeatedly(Invoke([this]() -> const Page& {
          return profile_.pages.at(current_id_);
        }));
  }

  Page& AddPage(std::u16string title) {
    Page page;
    page.title = std::move(title);
    return profile_.AddPage(page);
  }

  RailPagesController& MakeController() {
    controller_ =
        std::make_unique<RailPagesController>(RailPagesControllerContext{
            .activity_bar_ = bar_,
            .page_switcher_ = {.executor_ = executor_,
                               .profile_ = profile_,
                               .main_window_ = main_window_,
                               .main_window_manager_ = main_window_manager_,
                               .dialog_service_ = dialog_service_},
            .execute_command_ =
                [this](unsigned command_id) {
                  executed_.push_back(command_id);
                },
            .menu_parent_ = parent_});
    return *controller_;
  }

  // The rail's page buttons, by their tooltip — `N · Title`, the one thing
  // that tells a page button from the "+".
  std::vector<QString> PageTooltips() const {
    std::vector<QString> tooltips;
    for (QToolButton* button : bar_.findChildren<QToolButton*>()) {
      if (button->defaultAction() && button->toolTip().contains(u'·'))
        tooltips.push_back(button->toolTip());
    }
    return tooltips;
  }

  static QAction* FindAction(const QMenu& menu, const char* label) {
    const QString text = QString::fromStdU16String(Translate(label));
    for (QAction* action : menu.actions()) {
      if (action->text() == text)
        return action;
    }
    return nullptr;
  }

  AppEnvironment app_env_;
  TestExecutor executor_;
  Profile profile_;
  StrictMock<MockFunction<std::unique_ptr<MainWindow>(int window_id)>>
      main_window_factory_;
  StrictMock<MockFunction<void()>> quit_handler_;
  MainWindowManager main_window_manager_{{profile_,
                                          main_window_factory_.AsStdFunction(),
                                          quit_handler_.AsStdFunction()}};
  NiceMock<MockMainWindow> main_window_;
  StrictMock<MockDialogService> dialog_service_;
  QWidget parent_;
  ActivityBar bar_{&parent_, {}, [](PaneModeId) {}};

  int first_id_ = 0;
  int second_id_ = 0;
  int third_id_ = 0;
  int current_id_ = 0;
  std::vector<unsigned> executed_;
  std::unique_ptr<RailPagesController> controller_;
};

TEST_F(RailPagesControllerTest, BuildsOneButtonPerPageOnConstruction) {
  current_id_ = first_id_;
  MakeController();

  EXPECT_THAT(PageTooltips(),
              UnorderedElementsAre(QStringLiteral("1 · First"),
                                   QStringLiteral("2 · Second"),
                                   QStringLiteral("3 · Third")));
}

TEST_F(RailPagesControllerTest, RefreshFollowsTheProfile) {
  current_id_ = first_id_;
  RailPagesController& controller = MakeController();

  profile_.pages.at(second_id_).title = u"Renamed";
  controller.Refresh();

  EXPECT_THAT(PageTooltips(), Contains(QStringLiteral("2 · Renamed")));
}

// Rename, Duplicate and Delete act on the CURRENT page, so on any other page
// they are disabled rather than silently acting on the wrong one; Open leads.
TEST_F(RailPagesControllerTest, MenuOnAnotherPageOffersOpenAndDisablesEdits) {
  current_id_ = first_id_;
  RailPagesController& controller = MakeController();

  std::unique_ptr<QMenu> menu = controller.BuildPageContextMenu(second_id_);

  QAction* open = FindAction(*menu, "Open page");
  ASSERT_NE(open, nullptr);
  EXPECT_EQ(menu->defaultAction(), open);
  EXPECT_FALSE(FindAction(*menu, "Rename")->isEnabled());
  EXPECT_FALSE(FindAction(*menu, "Duplicate")->isEnabled());
  EXPECT_FALSE(FindAction(*menu, "Delete page")->isEnabled());
  EXPECT_TRUE(FindAction(*menu, "New page")->isEnabled());
}

TEST_F(RailPagesControllerTest, MenuOnTheCurrentPageEditsIt) {
  current_id_ = first_id_;
  RailPagesController& controller = MakeController();

  std::unique_ptr<QMenu> menu = controller.BuildPageContextMenu(first_id_);

  EXPECT_EQ(FindAction(*menu, "Open page"), nullptr);
  QAction* rename = FindAction(*menu, "Rename");
  ASSERT_TRUE(rename->isEnabled());
  rename->trigger();
  EXPECT_THAT(executed_, ElementsAre(ID_PAGE_RENAME));
}

// Each end of the list disables its own direction.
TEST_F(RailPagesControllerTest, MoveItemsFollowThePagesPosition) {
  current_id_ = first_id_;
  RailPagesController& controller = MakeController();

  std::unique_ptr<QMenu> first = controller.BuildPageContextMenu(first_id_);
  EXPECT_FALSE(FindAction(*first, "Move up")->isEnabled());
  EXPECT_TRUE(FindAction(*first, "Move down")->isEnabled());

  std::unique_ptr<QMenu> last = controller.BuildPageContextMenu(third_id_);
  EXPECT_TRUE(FindAction(*last, "Move up")->isEnabled());
  EXPECT_FALSE(FindAction(*last, "Move down")->isEnabled());
}

TEST_F(RailPagesControllerTest, MoveDownReordersAndRedrawsTheRail) {
  current_id_ = first_id_;
  RailPagesController& controller = MakeController();

  std::unique_ptr<QMenu> menu = controller.BuildPageContextMenu(first_id_);
  FindAction(*menu, "Move down")->trigger();

  EXPECT_THAT(PageTooltips(),
              UnorderedElementsAre(QStringLiteral("1 · Second"),
                                   QStringLiteral("2 · First"),
                                   QStringLiteral("3 · Third")));
}

// The icon is a property of the page, so it is settable on a page that is not
// open, and the check mark reads back from the profile.
TEST_F(RailPagesControllerTest, IconSubmenuSetsAndReflectsThePagesIcon) {
  ASSERT_FALSE(GetPageIcons().empty());
  const PageIcon& icon = GetPageIcons().front();
  current_id_ = first_id_;
  RailPagesController& controller = MakeController();

  std::unique_ptr<QMenu> menu = controller.BuildPageContextMenu(second_id_);
  QMenu* icons = FindAction(*menu, "Icon")->menu();
  ASSERT_NE(icons, nullptr);
  EXPECT_TRUE(FindAction(*icons, "None")->isChecked());
  FindAction(*icons, icon.label)->trigger();

  EXPECT_EQ(controller.PageIconFor(second_id_), icon.key);
  EXPECT_EQ(profile_.pages.at(second_id_).icon, icon.key);

  std::unique_ptr<QMenu> again = controller.BuildPageContextMenu(second_id_);
  QMenu* icons_again = FindAction(*again, "Icon")->menu();
  EXPECT_FALSE(FindAction(*icons_again, "None")->isChecked());
  EXPECT_TRUE(FindAction(*icons_again, icon.label)->isChecked());
}

TEST_F(RailPagesControllerTest, NewPageRunsTheRegisteredCommand) {
  current_id_ = first_id_;
  RailPagesController& controller = MakeController();

  std::unique_ptr<QMenu> menu = controller.BuildPageContextMenu(first_id_);
  FindAction(*menu, "New page")->trigger();

  EXPECT_THAT(executed_, ElementsAre(ID_PAGE_NEW));
}

}  // namespace
