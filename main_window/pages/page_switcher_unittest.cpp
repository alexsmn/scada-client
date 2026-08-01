#include "main_window/pages/page_switcher.h"

#include "aui/dialog_service_mock.h"
#include "aui/test/app_environment.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/main_window_mock.h"
#include "profile/profile.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

Awaitable<MessageBoxResult> ReturnMessageBoxResultAsync(
    MessageBoxResult result) {
  co_return result;
}

class PageSwitcherTest : public Test {
 protected:
  Page& AddPage(std::u16string title) {
    Page page;
    page.title = std::move(title);
    return profile_.AddPage(page);
  }

  PageSwitcher MakeSwitcher() {
    return PageSwitcher{
        PageSwitcherContext{.executor_ = executor_,
                            .profile_ = profile_,
                            .main_window_ = main_window_,
                            .main_window_manager_ = main_window_manager_,
                            .dialog_service_ = dialog_service_}};
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

  StrictMock<MockMainWindow> main_window_;
  StrictMock<MockDialogService> dialog_service_;
};

TEST_F(PageSwitcherTest, ListsPagesInIdOrder) {
  Page& first = AddPage(u"First");
  Page& second = AddPage(u"Second");
  EXPECT_CALL(main_window_, GetCurrentPage()).WillRepeatedly(ReturnRef(first));

  PageSwitcher switcher = MakeSwitcher();
  const std::vector<PageEntry> entries = switcher.ListPages();

  ASSERT_EQ(entries.size(), 2u);
  EXPECT_EQ(entries[0].page_id, first.id);
  EXPECT_EQ(entries[0].title, u"First");
  EXPECT_EQ(entries[1].page_id, second.id);
  EXPECT_EQ(entries[1].title, u"Second");
}

// Profile::AddPage auto-titles, but a page loaded from JSON without a "title"
// key has none — the menu used to render those as blank rows.
TEST_F(PageSwitcherTest, UntitledPageUsesItsSynthesizedTitle) {
  Page& page = AddPage(u"");
  page.title.clear();
  page.AddWindow(WindowDefinition{std::string_view{"Graph"}});
  EXPECT_CALL(main_window_, GetCurrentPage()).WillRepeatedly(ReturnRef(page));

  PageSwitcher switcher = MakeSwitcher();
  const std::vector<PageEntry> entries = switcher.ListPages();

  ASSERT_EQ(entries.size(), 1u);
  EXPECT_FALSE(entries[0].title.empty());
  EXPECT_EQ(entries[0].title, page.GetTitle());
}

TEST_F(PageSwitcherTest, MarksTheCurrentPage) {
  AddPage(u"First");
  Page& second = AddPage(u"Second");
  EXPECT_CALL(main_window_, GetCurrentPage()).WillRepeatedly(ReturnRef(second));

  PageSwitcher switcher = MakeSwitcher();
  const std::vector<PageEntry> entries = switcher.ListPages();

  ASSERT_EQ(entries.size(), 2u);
  EXPECT_FALSE(entries[0].current);
  EXPECT_TRUE(entries[1].current);
}

TEST_F(PageSwitcherTest, SwitchingToAnotherPageSavesTheCurrentOneFirst) {
  Page& current = AddPage(u"Current");
  Page& target = AddPage(u"Target");
  EXPECT_CALL(main_window_, GetCurrentPage())
      .WillRepeatedly(ReturnRef(current));

  InSequence sequence;
  EXPECT_CALL(main_window_, SaveCurrentPage());
  EXPECT_CALL(main_window_, OpenPage(Ref(target)));

  PageSwitcher switcher = MakeSwitcher();
  switcher.ActivatePage(target.id);
  Drain(executor_);
}

TEST_F(PageSwitcherTest, ActivatingTheCurrentPageConfirmsTheRevert) {
  Page& current = AddPage(u"Current");
  EXPECT_CALL(main_window_, GetCurrentPage())
      .WillRepeatedly(ReturnRef(current));
  EXPECT_CALL(dialog_service_,
              RunMessageBox(_, _, MessageBoxMode::QuestionYesNo))
      .WillOnce([](std::u16string_view, std::u16string_view, MessageBoxMode) {
        return ReturnMessageBoxResultAsync(MessageBoxResult::Yes);
      });
  // A revert discards unsaved changes, so the outgoing page must NOT be saved.
  EXPECT_CALL(main_window_, SaveCurrentPage()).Times(0);
  EXPECT_CALL(main_window_, OpenPage(Ref(current)));

  PageSwitcher switcher = MakeSwitcher();
  switcher.ActivatePage(current.id);
  Drain(executor_);
}

TEST_F(PageSwitcherTest, CancelingTheRevertLeavesThePageAlone) {
  Page& current = AddPage(u"Current");
  EXPECT_CALL(main_window_, GetCurrentPage())
      .WillRepeatedly(ReturnRef(current));
  EXPECT_CALL(dialog_service_,
              RunMessageBox(_, _, MessageBoxMode::QuestionYesNo))
      .WillOnce([](std::u16string_view, std::u16string_view, MessageBoxMode) {
        return ReturnMessageBoxResultAsync(MessageBoxResult::No);
      });
  EXPECT_CALL(main_window_, OpenPage(_)).Times(0);

  PageSwitcher switcher = MakeSwitcher();
  switcher.ActivatePage(current.id);
  Drain(executor_);
}

// Reordering is what the rail's drag-and-drop drives, and ListPages() is the
// single list both the rail and the Page menu read — so ordering the list here
// is what keeps the two surfaces in step.
TEST_F(PageSwitcherTest, ReorderPageMovesAPageAndRenumbersTheRest) {
  Page& first = AddPage(u"First");
  Page& second = AddPage(u"Second");
  Page& third = AddPage(u"Third");
  EXPECT_CALL(main_window_, GetCurrentPage()).WillRepeatedly(ReturnRef(first));

  PageSwitcher switcher = MakeSwitcher();
  switcher.ReorderPage(third.id, 0);

  const std::vector<PageEntry> entries = switcher.ListPages();
  ASSERT_EQ(entries.size(), 3u);
  EXPECT_EQ(entries[0].page_id, third.id);
  EXPECT_EQ(entries[1].page_id, first.id);
  EXPECT_EQ(entries[2].page_id, second.id);
}

TEST_F(PageSwitcherTest, ReorderPageSurvivesInTheProfile) {
  Page& first = AddPage(u"First");
  Page& second = AddPage(u"Second");
  EXPECT_CALL(main_window_, GetCurrentPage()).WillRepeatedly(ReturnRef(first));

  PageSwitcher switcher = MakeSwitcher();
  switcher.ReorderPage(second.id, 0);

  // Dense 1-based orders, so a later AddPage lands after them.
  EXPECT_EQ(profile_.pages.at(second.id).order, 1);
  EXPECT_EQ(profile_.pages.at(first.id).order, 2);
}

TEST_F(PageSwitcherTest, ANewPageLandsAtTheEndOfTheOrder) {
  Page& first = AddPage(u"First");
  Page& second = AddPage(u"Second");
  EXPECT_CALL(main_window_, GetCurrentPage()).WillRepeatedly(ReturnRef(first));

  PageSwitcher switcher = MakeSwitcher();
  switcher.ReorderPage(second.id, 0);
  Page& added = AddPage(u"Added");

  const std::vector<PageEntry> entries = switcher.ListPages();
  ASSERT_EQ(entries.size(), 3u);
  EXPECT_EQ(entries.back().page_id, added.id);
}

// A profile written before reordering existed has every order at 0; it must
// keep the id order it has always had rather than shuffling on upgrade.
TEST_F(PageSwitcherTest, UnorderedPagesKeepTheirHistoricalIdOrder) {
  Page& first = AddPage(u"First");
  Page& second = AddPage(u"Second");
  Page& third = AddPage(u"Third");
  EXPECT_CALL(main_window_, GetCurrentPage()).WillRepeatedly(ReturnRef(first));

  PageSwitcher switcher = MakeSwitcher();
  const std::vector<PageEntry> entries = switcher.ListPages();

  ASSERT_EQ(entries.size(), 3u);
  EXPECT_EQ(entries[0].page_id, first.id);
  EXPECT_EQ(entries[1].page_id, second.id);
  EXPECT_EQ(entries[2].page_id, third.id);
}

TEST_F(PageSwitcherTest, ReorderPageIgnoresAnUnknownId) {
  Page& first = AddPage(u"First");
  AddPage(u"Second");
  EXPECT_CALL(main_window_, GetCurrentPage()).WillRepeatedly(ReturnRef(first));

  PageSwitcher switcher = MakeSwitcher();
  switcher.ReorderPage(9999, 0);

  EXPECT_EQ(switcher.ListPages()[0].page_id, first.id);
}

TEST_F(PageSwitcherTest, ActivatingAnUnknownPageIdDoesNothing) {
  Page& current = AddPage(u"Current");
  EXPECT_CALL(main_window_, GetCurrentPage())
      .Times(AnyNumber())
      .WillRepeatedly(ReturnRef(current));
  EXPECT_CALL(main_window_, OpenPage(_)).Times(0);

  PageSwitcher switcher = MakeSwitcher();
  switcher.ActivatePage(current.id + 100);
  Drain(executor_);
}

TEST_F(PageSwitcherTest, SetPageIconStoresTheKeyAndListsIt) {
  Page& page = AddPage(u"Alarms");
  EXPECT_CALL(main_window_, GetCurrentPage()).WillRepeatedly(ReturnRef(page));

  PageSwitcher switcher = MakeSwitcher();
  switcher.SetPageIcon(page.id, "alarms");

  EXPECT_EQ(page.icon, "alarms");
  ASSERT_EQ(switcher.ListPages().size(), 1u);
  EXPECT_EQ(switcher.ListPages()[0].icon, "alarms");
}

TEST_F(PageSwitcherTest, SetPageIconRejectsAKeyThisBuildCannotDraw) {
  Page& page = AddPage(u"Alarms");
  EXPECT_CALL(main_window_, GetCurrentPage()).WillRepeatedly(ReturnRef(page));
  page.icon = "alarms";

  PageSwitcher switcher = MakeSwitcher();
  switcher.SetPageIcon(page.id, "not-a-glyph");

  // Persisting a key the rail cannot draw would leave the page rendering as
  // its ordinal with nothing to explain why, so the old key survives.
  EXPECT_EQ(page.icon, "alarms");
}

TEST_F(PageSwitcherTest, SetPageIconWithAnEmptyKeyClearsTheIcon) {
  Page& page = AddPage(u"Alarms");
  EXPECT_CALL(main_window_, GetCurrentPage()).WillRepeatedly(ReturnRef(page));
  page.icon = "alarms";

  PageSwitcher switcher = MakeSwitcher();
  switcher.SetPageIcon(page.id, {});

  EXPECT_TRUE(page.icon.empty());
}

// Page's copy constructor and assignment operator listed their members by
// hand, and `order` was never added to either when reordering landed — so
// copying a page silently reset its rail position to "unordered". `icon` would
// have gone the same way. Everything that round-trips a page through a copy
// (Profile::AddPage, duplication) depends on this.
TEST(PageTest, CopyingKeepsOrderAndIcon) {
  Page source;
  source.title = u"Trends";
  source.icon = "trend";
  source.order = 4;

  const Page copied{source};
  EXPECT_EQ(copied.icon, "trend");
  EXPECT_EQ(copied.order, 4);

  Page assigned;
  assigned = source;
  EXPECT_EQ(assigned.icon, "trend");
  EXPECT_EQ(assigned.order, 4);
}

// Profile::AddPage lands a new page at the end of the operator's order, so it
// assigns `order` itself rather than trusting the source's. The icon is the
// operator's choice and is carried across untouched.
TEST_F(PageSwitcherTest, AddingAPageKeepsItsIcon) {
  Page source;
  source.title = u"Trends";
  source.icon = "trend";

  const Page& added = profile_.AddPage(source);

  EXPECT_EQ(added.icon, "trend");
}

}  // namespace
