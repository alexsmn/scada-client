#include "window_definition_util.h"

#include <gmock/gmock.h>

using namespace testing;

TEST(NodeId, FromJson) {
  EXPECT_FALSE(FromJson<scada::NodeId>(base::Value()));
  EXPECT_FALSE(FromJson<scada::NodeId>(base::Value(123)));
  EXPECT_FALSE(FromJson<scada::NodeId>(base::Value("abcdef")));
  EXPECT_EQ(scada::NodeId(123, 1),
            FromJson<scada::NodeId>(base::Value("TS.123")));
}

TEST(NodeId, ToJson) {
  EXPECT_EQ(base::Value(), ToJson(scada::NodeId()));
  EXPECT_EQ(base::Value("TS.123"), ToJson(scada::NodeId(123, 1)));
}
