#include "controller/action_manager.h"

#include "controller/action.h"

#include <gtest/gtest.h>

namespace {

// The Explorer context menu and the command toolbar draw the seven
// CATEGORY_OPEN view commands differently on purpose --
// docs/product/ui-mockups/authoring.md 4b "Opening a view" -- and one predicate
// served both surfaces until 2026-09-09, so the toolbar shipped seven
// unlabelled 16px glyphs where the screens draw one labelled `Open` button.
// Pin both answers: making the toolbar collapse the group must not bury the
// context menu's commands behind a submenu.
TEST(CommandSurfaceTest, OpenExpandsInTheContextMenuAndCollapsesInTheToolbar) {
  EXPECT_TRUE(
      CanExpandCommandCategory(CATEGORY_OPEN, CommandSurface::kContextMenu));
  EXPECT_FALSE(
      CanExpandCommandCategory(CATEGORY_OPEN, CommandSurface::kToolbar));
}

// CATEGORY_OPEN is the only category the two surfaces disagree about. A second
// divergence is a decision about the screens, so it must not arrive by
// accident: every other category answers the same on both.
TEST(CommandSurfaceTest, OpenIsTheOnlyCategoryTheSurfacesDisagreeAbout) {
  for (int i = 0; i < CATEGORY_COUNT; ++i) {
    const auto category = static_cast<CommandCategory>(i);
    if (category == CATEGORY_OPEN) {
      continue;
    }
    EXPECT_EQ(CanExpandCommandCategory(category, CommandSurface::kContextMenu),
              CanExpandCommandCategory(category, CommandSurface::kToolbar))
        << "category " << i;
  }
}

// The categories that were already collapsed stay collapsed, on both surfaces.
// This is the half a change to the CATEGORY_OPEN branch could silently drop.
TEST(CommandSurfaceTest, TheAlreadyCollapsedCategoriesStayCollapsed) {
  for (CommandCategory category :
       {CATEGORY_CREATE, CATEGORY_DEVICE, CATEGORY_PERIOD, CATEGORY_NEW,
        CATEGORY_AGGREGATION, CATEGORY_INTERVAL, CATEGORY_EXPORT}) {
    EXPECT_FALSE(
        CanExpandCommandCategory(category, CommandSurface::kContextMenu))
        << "category " << static_cast<int>(category);
    EXPECT_FALSE(CanExpandCommandCategory(category, CommandSurface::kToolbar))
        << "category " << static_cast<int>(category);
  }
}

// A collapsed group is drawn as a button carrying the category's title, so the
// title has to be there for every category that collapses -- on either surface.
TEST(CommandSurfaceTest, EveryCollapsedCategoryHasATitle) {
  for (int i = 0; i < CATEGORY_COUNT; ++i) {
    const auto category = static_cast<CommandCategory>(i);
    if (CanExpandCommandCategory(category, CommandSurface::kContextMenu) &&
        CanExpandCommandCategory(category, CommandSurface::kToolbar)) {
      continue;
    }
    EXPECT_FALSE(GetCommandCategoryTitle(category).empty()) << "category " << i;
  }
}

}  // namespace
