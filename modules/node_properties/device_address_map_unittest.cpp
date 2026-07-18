#include "node_properties/device_address_map.h"

#include "model/data_items_node_ids.h"
#include "scada/node_id.h"

#include <gtest/gtest.h>

namespace {

TEST(SignalTypeTagTest, DiscreteItemIsTS) {
  EXPECT_EQ(SignalTypeTag(scada::data_items::id::DiscreteItemType), u"TS");
}

TEST(SignalTypeTagTest, AnalogItemIsTI) {
  EXPECT_EQ(SignalTypeTag(scada::data_items::id::AnalogItemType), u"TI");
}

TEST(SignalTypeTagTest, OtherTypeHasNoTag) {
  EXPECT_TRUE(SignalTypeTag(scada::NodeId{}).empty());
  EXPECT_TRUE(SignalTypeTag(scada::data_items::id::DataItemType).empty());
}

}  // namespace
