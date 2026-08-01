#include "main_window/activity_bar_qt.h"

#include "aui/test/app_environment.h"
#include "main_window/page_icons.h"

#include <QApplication>
#include <QImage>
#include <QToolButton>

#include <gtest/gtest.h>

#include <optional>
#include <vector>

namespace {

std::vector<ActivityBar::Mode> MakeModes() {
  return {
      {PaneModeId::kObjects, u"Objects", ActivityBar::Icon::kObjects},
      {PaneModeId::kDevices, u"Devices", ActivityBar::Icon::kDevices},
      {PaneModeId::kFiles, u"Files", ActivityBar::Icon::kFiles},
      {PaneModeId::kNodes, u"Nodes", ActivityBar::Icon::kNodes},
  };
}

// Every rail button, in creation order: the four pane modes, then the "+",
// then the page buttons (SetPages appends them after construction).
std::vector<QToolButton*> AllButtons(const ActivityBar& bar) {
  const QList<QToolButton*> found = bar.findChildren<QToolButton*>();
  return {found.begin(), found.end()};
}

// Just the pane-mode buttons.
std::vector<QToolButton*> Buttons(const ActivityBar& bar) {
  std::vector<QToolButton*> all = AllButtons(bar);
  all.resize(4);
  return all;
}

// Just the page buttons, identified by their numeric tooltips being page
// titles — they are everything after the four modes and the "+".
std::vector<QToolButton*> PageButtons(const ActivityBar& bar) {
  std::vector<QToolButton*> all = AllButtons(bar);
  return std::vector<QToolButton*>{all.begin() + 5, all.end()};
}

std::vector<ActivityBar::PageButton> MakePages() {
  return {
      {.page_id = 7, .title = u"Overview"},
      {.page_id = 9, .title = u"Trends"},
      {.page_id = 11, .title = u"Locked", .opened_elsewhere = true},
  };
}

class ActivityBarTest : public ::testing::Test {
 protected:
  // Per-test QApplication (Qt requires one before any QWidget), destroyed
  // with the fixture. Never keep a static QApplication in a test binary: it
  // is destroyed during atexit teardown, where ~QGuiApplication crashes on
  // macOS after other Qt statics are already gone.
  AppEnvironment app_env_;
};

TEST_F(ActivityBarTest, ClickingAModeActivatesIt) {
  std::vector<PaneModeId> activated;
  ActivityBar bar{nullptr, MakeModes(),
                  [&](PaneModeId id) { activated.push_back(id); }};

  const std::vector<QToolButton*> buttons = Buttons(bar);
  ASSERT_EQ(buttons.size(), 4u);

  buttons[2]->click();

  EXPECT_EQ(activated, (std::vector<PaneModeId>{PaneModeId::kFiles}));
}

TEST_F(ActivityBarTest, SetActiveModeChecksTheMatchingButton) {
  ActivityBar bar{nullptr, MakeModes(), {}};
  const std::vector<QToolButton*> buttons = Buttons(bar);
  ASSERT_EQ(buttons.size(), 4u);

  bar.SetActiveMode(PaneModeId::kDevices);

  EXPECT_FALSE(buttons[0]->isChecked());
  EXPECT_TRUE(buttons[1]->isChecked());
  EXPECT_FALSE(buttons[2]->isChecked());
}

// The rail must be able to say "no mode": the open panes can be in a
// combination that matches none, and an exclusive button group cannot express
// that.
TEST_F(ActivityBarTest, SetActiveModeWithNoModeUnchecksEveryButton) {
  ActivityBar bar{nullptr, MakeModes(), {}};
  bar.SetActiveMode(PaneModeId::kObjects);

  bar.SetActiveMode(std::nullopt);

  for (QToolButton* button : Buttons(bar))
    EXPECT_FALSE(button->isChecked());
}

TEST_F(ActivityBarTest, SetActiveModeMovesTheMarkerRatherThanAddingOne) {
  ActivityBar bar{nullptr, MakeModes(), {}};

  bar.SetActiveMode(PaneModeId::kObjects);
  bar.SetActiveMode(PaneModeId::kNodes);

  const std::vector<QToolButton*> buttons = Buttons(bar);
  EXPECT_FALSE(buttons[0]->isChecked());
  EXPECT_TRUE(buttons[3]->isChecked());
}

// Nodes is admin-only. A hidden button says "not yours"; a disabled one would
// promise a surface that is merely unfinished.
TEST_F(ActivityBarTest, SetModeAvailableHidesTheButton) {
  ActivityBar bar{nullptr, MakeModes(), {}};
  const std::vector<QToolButton*> buttons = Buttons(bar);

  bar.SetModeAvailable(PaneModeId::kNodes, false);
  EXPECT_TRUE(buttons[3]->isHidden());

  bar.SetModeAvailable(PaneModeId::kNodes, true);
  EXPECT_FALSE(buttons[3]->isHidden());
}

TEST_F(ActivityBarTest, HidingTheActiveModeClearsItsMarker) {
  ActivityBar bar{nullptr, MakeModes(), {}};
  bar.SetActiveMode(PaneModeId::kNodes);

  bar.SetModeAvailable(PaneModeId::kNodes, false);

  EXPECT_FALSE(Buttons(bar)[3]->isChecked());
}

// Pages numbered by rail position: titles are arbitrary and will not fit a
// 52 px rail, so the number is the label and the title is the tooltip.
TEST_F(ActivityBarTest, PageTooltipCarriesTheOrdinalAndTheTitle) {
  ActivityBar bar{nullptr, MakeModes(), {}};
  bar.SetPages(MakePages());

  const std::vector<QToolButton*> pages = PageButtons(bar);
  ASSERT_EQ(pages.size(), 3u);
  // The button shows an icon, so the tooltip is the only place the ordinal
  // (which names the shortcut) and the title are legible.
  EXPECT_EQ(pages[0]->toolTip(), "1 · Overview");
  EXPECT_EQ(pages[1]->toolTip(), "2 · Trends");
}

TEST_F(ActivityBarTest, PageWithAnIconRendersItRatherThanTheOrdinal) {
  ActivityBar bar{nullptr, MakeModes(), {}};
  bar.SetPages({{.page_id = 1, .title = u"Alarms", .icon_key = "alarms"},
                {.page_id = 2, .title = u"Plain"}});

  const std::vector<QToolButton*> pages = PageButtons(bar);
  ASSERT_EQ(pages.size(), 2u);
  // Both draw something; what matters is that they draw something *different*
  // — otherwise the icon silently fell back to the ordinal.
  EXPECT_NE(pages[0]->icon().pixmap(24, 24).toImage(),
            pages[1]->icon().pixmap(24, 24).toImage());
}

TEST_F(ActivityBarTest, UnknownPageIconFallsBackToTheOrdinal) {
  ActivityBar bar{nullptr, MakeModes(), {}};
  // A profile written by a newer build can name an icon this one cannot draw.
  // It must degrade to the numbered button, not to a blank one.
  bar.SetPages({{.page_id = 1, .title = u"Future", .icon_key = "not-a-glyph"},
                {.page_id = 2, .title = u"Plain"}});

  const std::vector<QToolButton*> pages = PageButtons(bar);
  ASSERT_EQ(pages.size(), 2u);
  EXPECT_FALSE(pages[0]->icon().pixmap(24, 24).isNull());
  // Page 2's own ordinal differs, so compare against a rail whose first page
  // has no icon at all: same slot, same ordinal, so the images must match.
  ActivityBar plain{nullptr, MakeModes(), {}};
  plain.SetPages({{.page_id = 1, .title = u"Future"}});
  EXPECT_EQ(pages[0]->icon().pixmap(24, 24).toImage(),
            PageButtons(plain)[0]->icon().pixmap(24, 24).toImage());
}

TEST_F(ActivityBarTest, EveryPickablePageIconHasAGlyph) {
  // page_icons.h offers the operator a list; the rail maps each key to a
  // Lucide asset. A key added to one and not the other would draw the ordinal
  // for ever with nothing to say why, so hold the two in step here.
  ActivityBar unknown{nullptr, MakeModes(), {}};
  unknown.SetPages({{.page_id = 1, .title = u"X", .icon_key = "not-a-glyph"}});
  const QImage ordinal =
      PageButtons(unknown)[0]->icon().pixmap(24, 24).toImage();

  for (const PageIcon& icon : GetPageIcons()) {
    ActivityBar bar{nullptr, MakeModes(), {}};
    bar.SetPages(
        {{.page_id = 1, .title = u"X", .icon_key = std::string{icon.key}}});
    EXPECT_NE(PageButtons(bar)[0]->icon().pixmap(24, 24).toImage(), ordinal)
        << "page icon key '" << icon.key << "' has no glyph in the rail";
  }
}

TEST_F(ActivityBarTest, PageOpenedInAnotherWindowIsDisabled) {
  ActivityBar bar{nullptr, MakeModes(), {}};
  bar.SetPages(MakePages());

  const std::vector<QToolButton*> pages = PageButtons(bar);
  ASSERT_EQ(pages.size(), 3u);
  EXPECT_TRUE(pages[0]->isEnabled());
  EXPECT_FALSE(pages[2]->isEnabled());
}

TEST_F(ActivityBarTest, ClickingAPageRequestsThatPage) {
  std::vector<int> activated;
  ActivityBar bar{nullptr, MakeModes(), {}};
  bar.SetPageCallbacks([&](int page_id) { activated.push_back(page_id); }, {},
                       {}, {});
  bar.SetPages(MakePages());

  PageButtons(bar)[1]->click();

  EXPECT_EQ(activated, (std::vector<int>{9}));
}

TEST_F(ActivityBarTest, NewPageButtonRequestsANewPage) {
  int new_pages = 0;
  ActivityBar bar{nullptr, MakeModes(), {}};
  bar.SetPageCallbacks({}, [&] { ++new_pages; }, {}, {});
  bar.SetPages(MakePages());

  // The "+" sits between the mode group and the page buttons.
  AllButtons(bar)[4]->click();

  EXPECT_EQ(new_pages, 1);
}

// A pane mode and a page are active at the same time, so the two groups carry
// independent markers.
TEST_F(ActivityBarTest, PageAndModeMarkersAreIndependent) {
  ActivityBar bar{nullptr, MakeModes(), {}};
  bar.SetPages(MakePages());

  bar.SetActiveMode(PaneModeId::kFiles);
  bar.SetActivePage(9);

  EXPECT_TRUE(Buttons(bar)[2]->isChecked());
  EXPECT_FALSE(PageButtons(bar)[0]->isChecked());
  EXPECT_TRUE(PageButtons(bar)[1]->isChecked());
}

// SetPages destroys and rebuilds the buttons, so the marker has to be
// re-asserted or a page switch would silently unmark the open page.
TEST_F(ActivityBarTest, RebuildingPagesKeepsTheActivePageMarked) {
  ActivityBar bar{nullptr, MakeModes(), {}};
  bar.SetPages(MakePages());
  bar.SetActivePage(9);

  std::vector<ActivityBar::PageButton> more = MakePages();
  more.push_back({.page_id = 13, .title = u"Added"});
  bar.SetPages(std::move(more));

  const std::vector<QToolButton*> pages = PageButtons(bar);
  ASSERT_EQ(pages.size(), 4u);
  EXPECT_TRUE(pages[1]->isChecked());
}

TEST_F(ActivityBarTest, TheNewPageButtonNeverCarriesAMarker) {
  ActivityBar bar{nullptr, MakeModes(), {}};
  bar.SetPages(MakePages());
  bar.SetActivePage(7);

  EXPECT_FALSE(AllButtons(bar)[4]->isCheckable());
}

// Drag-and-drop reordering: the drop index comes from where the cursor sits
// relative to the page buttons' midpoints. Synthesizing a real Qt drag in a
// unit test is unreliable, so the geometry rule is tested directly.
TEST_F(ActivityBarTest, PageDropIndexFollowsButtonMidpoints) {
  ActivityBar bar{nullptr, MakeModes(), {}};
  bar.SetPages(MakePages());
  // The layout only assigns geometry once the widget has been laid out.
  bar.resize(52, 600);
  bar.show();
  QApplication::processEvents();

  const std::vector<QToolButton*> pages = PageButtons(bar);
  ASSERT_EQ(pages.size(), 3u);

  // In rail coordinates, which is what PageDropIndexForY takes. The buttons
  // are children of the pages band, so their own geometry() is offset by the
  // band's position — reading it directly puts every drop a slot out.
  const auto midpoint = [&bar](QToolButton* button) {
    return button->mapTo(&bar, QPoint{0, 0}).y() + button->height() / 2;
  };

  EXPECT_EQ(bar.PageDropIndexForY(midpoint(pages[0]) - 4), 0);
  EXPECT_EQ(bar.PageDropIndexForY(midpoint(pages[0]) + 4), 1);
  EXPECT_EQ(bar.PageDropIndexForY(midpoint(pages[2]) + 4), 3);
}

TEST_F(ActivityBarTest, EveryModeRendersItsDedicatedGlyph) {
  ActivityBar bar{nullptr, MakeModes(), {}};

  for (QToolButton* button : Buttons(bar)) {
    ASSERT_FALSE(button->icon().isNull());
    // A dedicated glyph paints something; the letter fallback would too, but
    // an empty pixmap would mean the Icon enumerator has no case.
    EXPECT_FALSE(button->icon().pixmap(24, 24).isNull());
  }
}

}  // namespace
