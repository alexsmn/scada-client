#include "profile/page_layout.h"

#include "base/json.h"

#include <gmock/gmock.h>

TEST(PageLayout, Serialization) {
  PageLayout layout;
  layout.main.split(true);
  const auto json = ToJson(layout);
  auto restored_layout = FromJson<PageLayout>(json);
  EXPECT_TRUE(restored_layout.has_value());
  EXPECT_EQ(layout, *restored_layout);
}