#include "controller/action.h"

#include <gtest/gtest.h>

namespace {

// Regression: CHECKABLE must be its own bit, independent of VISIBLE and
// CHECKED. It used to be 0x0016 (= 0x10|VISIBLE|CHECKED), so toggling checkable
// spuriously set/cleared visible and checked, and any visible/checked action
// read back as checkable.
TEST(ActionTest, CheckableIsIndependentOfVisibleAndChecked) {
  Action action;

  action.set_checkable(true);
  EXPECT_TRUE(action.checkable());
  EXPECT_FALSE(action.visible());  // setting checkable must not set visible
  EXPECT_FALSE(action.checked());  // ... nor checked

  action.set_visible(true);
  action.set_checked(true);
  action.set_checkable(false);
  EXPECT_FALSE(action.checkable());
  EXPECT_TRUE(action.visible());  // clearing checkable must not clear visible
  EXPECT_TRUE(action.checked());  // ... nor checked
}

}  // namespace
