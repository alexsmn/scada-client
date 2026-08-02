#include "transmission_rules/transmission_rule.h"

#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "scada/node_id.h"

#include <gtest/gtest.h>

namespace {

TEST(TransmissionProtocolLabelTest, MapsKnownItemTypes) {
  EXPECT_EQ(TransmissionProtocolLabel(
                scada::devices::id::ModbusTransmissionItemType),
            u"Modbus");
  EXPECT_EQ(TransmissionProtocolLabel(
                scada::devices::id::Iec60870TransmissionItemType),
            u"IEC 60870-5-104");
  EXPECT_EQ(TransmissionProtocolLabel(
                scada::devices::id::Iec61850TransmissionItemType),
            u"IEC 61850");
}

TEST(TransmissionProtocolLabelTest, UnknownTypeIsEmpty) {
  EXPECT_TRUE(
      TransmissionProtocolLabel(scada::devices::id::TransmissionItemType)
          .empty());
}

TEST(TransmissionSignalTagTest, DiscreteIsTsAnalogIsTi) {
  EXPECT_EQ(TransmissionSignalTag(scada::data_items::id::DiscreteItemType),
            u"TS");
  EXPECT_EQ(TransmissionSignalTag(scada::data_items::id::AnalogItemType),
            u"TI");
}

TEST(TransmissionSignalTagTest, OtherTypeIsEmpty) {
  EXPECT_TRUE(
      TransmissionSignalTag(scada::devices::id::TransmissionItemType).empty());
}

TEST(TransmissionRuleSummaryTest, ComposesSourceAndAddress) {
  EXPECT_EQ(TransmissionRuleSummary(u"Ua", 2001), u"Ua → 2001");
}

TEST(TransmissionRuleSummaryTest, EmptySourceRendersDash) {
  EXPECT_EQ(TransmissionRuleSummary(u"", 4002), u"— → 4002");
}

}  // namespace
