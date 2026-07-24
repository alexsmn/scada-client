#include "main_window/status_bar/user_status_provider.h"

#include <gtest/gtest.h>

#include <string_view>

namespace {

TEST(UserRoleKeyTest, MapsAccessRightTiersToRole) {
  // Configure implies the top tier regardless of Control.
  EXPECT_EQ(std::string_view{UserRoleKey(/*can_configure=*/true,
                                         /*can_control=*/true)},
            "Administrator");
  EXPECT_EQ(std::string_view{UserRoleKey(/*can_configure=*/true,
                                         /*can_control=*/false)},
            "Administrator");
  // Control without Configure is the operator tier.
  EXPECT_EQ(std::string_view{UserRoleKey(/*can_configure=*/false,
                                         /*can_control=*/true)},
            "Operator");
  // No access rights is read-only.
  EXPECT_EQ(std::string_view{UserRoleKey(/*can_configure=*/false,
                                         /*can_control=*/false)},
            "Observer");
}

}  // namespace
