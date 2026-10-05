#include "controller/action_manager.h"

#include "controller/action.h"

#include <gtest/gtest.h>

namespace {

// The Explorer context menu draws the seven CATEGORY_OPEN view commands inline
// above the item commands -- docs/product/ui-mockups/authoring.md 4b "Opening
// a view" -- rather than behind an `Open` submenu.
TEST(CanExpandCommandCategoryTest, OpenExpands) {
  EXPECT_TRUE(CanExpandCommandCategory(CATEGORY_OPEN));
}

// The categories that collapse behind a titled submenu stay collapsed.
TEST(CanExpandCommandCategoryTest, TheGroupedCategoriesCollapse) {
  for (CommandCategory category :
       {CATEGORY_CREATE, CATEGORY_DEVICE, CATEGORY_PERIOD, CATEGORY_NEW,
        CATEGORY_AGGREGATION, CATEGORY_INTERVAL, CATEGORY_EXPORT}) {
    EXPECT_FALSE(CanExpandCommandCategory(category))
        << "category " << static_cast<int>(category);
  }
}

// A collapsed group is drawn as a submenu carrying the category's title, so
// the title has to be there for every category that collapses.
TEST(CanExpandCommandCategoryTest, EveryCollapsedCategoryHasATitle) {
  for (int i = 0; i < CATEGORY_COUNT; ++i) {
    const auto category = static_cast<CommandCategory>(i);
    if (CanExpandCommandCategory(category))
      continue;
    EXPECT_FALSE(GetCommandCategoryTitle(category).empty()) << "category " << i;
  }
}

}  // namespace
