#include "display_view/display_live_data.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "common/aliases_mock.h"
#include "common/variable_handle.h"
#include "events/node_event_provider_mock.h"
#include "node_service/static/static_node_service.h"
#include "scada/attribute_service.h"
#include "scada/monitored_item_service_mock.h"
#include "scada/standard_node_ids.h"
#include "timed_data/timed_data_service_impl.h"
#include "timed_data/timed_data_service_mock.h"
#include "timed_data/timed_data_spec.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

using testing::Return;

// What a schematic's value field is sent when there is nothing to show. It
// must never be empty: an empty text left the runtime to keep whatever the
// field last showed, which for a field that never got a reading is the
// designer's saved number (backlog 1005).

TEST(DisplayLiveDataTest, AnUnconnectedSourceReadsAsBadWithTheMark) {
  const TimedDataSpec spec;
  const DisplayDataSourceReading reading =
      DisplayLiveData::MakeReading("TIT_U", spec);

  EXPECT_EQ(reading.data_source, "TIT_U");
  EXPECT_FALSE(reading.value.has_value());
  EXPECT_EQ(reading.text, "?");
  EXPECT_NE(reading.quality, 0);
}

TEST(DisplayLiveDataTest, AConnectedSourceWithNoValueYetShowsTheMark) {
  testing::NiceMock<MockTimedDataService> service;
  TimedDataSpec spec;
  spec.Connect(service, "TIT_U");
  ASSERT_TRUE(spec.connected());
  ON_CALL(*service.default_timed_data_, GetDataValue())
      .WillByDefault(Return(scada::DataValue{}));

  const DisplayDataSourceReading reading =
      DisplayLiveData::MakeReading("TIT_U", spec);

  EXPECT_FALSE(reading.value.has_value());
  EXPECT_EQ(reading.text, "?");
  EXPECT_TRUE(reading.state.empty());
}

TEST(DisplayLiveDataTest, AValueIsSentAsANumberAndText) {
  testing::NiceMock<MockTimedDataService> service;
  TimedDataSpec spec;
  spec.Connect(service, "TIT_U");
  scada::DataValue value;
  value.value = 12.5;
  ON_CALL(*service.default_timed_data_, GetDataValue())
      .WillByDefault(Return(value));

  const DisplayDataSourceReading reading =
      DisplayLiveData::MakeReading("TIT_U", spec);

  ASSERT_TRUE(reading.value.has_value());
  EXPECT_DOUBLE_EQ(*reading.value, 12.5);
  EXPECT_FALSE(reading.text.empty());
  EXPECT_NE(reading.text, "?");
  EXPECT_EQ(reading.quality, 0);
}

// A schematic bound to a third-party server's node by namespace URI, as the
// Designer writes it, gets that node's live value through the real formula
// service: what a display opened on a foreign server shows
// (docs/product/foreign-node-references.md, phase 4). The index the
// connection gave the namespace is its own and appears nowhere in the binding.
class DisplayLiveDataForeignServerTest : public testing::Test {
 protected:
  DisplayLiveDataForeignServerTest() {
    node_service_.Add({.node_id = kPlantNodeId,
                       .node_class = scada::NodeClass::Variable,
                       .type_definition_id = scada::id::BaseVariableType});
    ON_CALL(monitored_item_service_,
            CreateMonitoredItem(testing::_, testing::_))
        .WillByDefault(Return(nullptr));
    ON_CALL(monitored_item_service_,
            CreateMonitoredItem(
                testing::FieldsAre(kPlantNodeId, scada::AttributeId::Value),
                testing::_))
        .WillByDefault(
            Return(scada::CreateMonitoredVariable(plant_value_variable_)));
  }

  static constexpr scada::NamespaceIndex kPlantNamespaceIndex = 200;
  inline static const scada::NodeId kPlantNodeId{
      std::string{"Plant.Temperature"}, kPlantNamespaceIndex};
  inline static const std::string kBinding =
      "{nsu=urn:plant;s=Plant.Temperature}";

  static std::optional<scada::NamespaceIndex> PlantNamespaceIndex(
      std::string_view uri) {
    if (uri == "urn:plant")
      return kPlantNamespaceIndex;
    return std::nullopt;
  }

  TestExecutor executor_;
  // Strict: a binding by URI must never be taken for an alias, which a foreign
  // server has no folder for.
  testing::StrictMock<MockAliasResolver> alias_resolver_;
  testing::NiceMock<scada::MockMonitoredItemService> monitored_item_service_;
  testing::NiceMock<MockNodeEventProvider> node_event_provider_;
  StaticNodeService node_service_{
      {.monitored_item_service = &monitored_item_service_,
       .monitored_item_executor = executor_}};
  TimedDataServiceImpl timed_data_service_{
      TimedDataContext{.executor_ = executor_,
                       .alias_resolver_ = alias_resolver_.AsStdFunction(),
                       .node_service_ = node_service_,
                       .node_event_provider_ = node_event_provider_,
                       .namespace_index_for_uri_ = PlantNamespaceIndex}};
  const std::shared_ptr<scada::VariableHandle> plant_value_variable_ =
      std::make_shared<scada::VariableHandle>();
};

TEST_F(DisplayLiveDataForeignServerTest, ABindingByUriShowsTheLiveValue) {
  plant_value_variable_->ForwardData(scada::MakeReadResult(42.5));

  std::vector<DisplayDataSourceReading> readings;
  const std::vector<std::string> sources{kBinding};
  DisplayLiveData live_data{timed_data_service_, sources,
                            [&](const DisplayDataSourceReading& reading) {
                              readings.push_back(reading);
                            }};
  Drain(executor_);

  ASSERT_FALSE(readings.empty());
  const DisplayDataSourceReading& last = readings.back();
  EXPECT_EQ(last.data_source, kBinding);
  ASSERT_TRUE(last.value.has_value());
  EXPECT_DOUBLE_EQ(*last.value, 42.5);
  EXPECT_EQ(last.quality, 0);
  // A click on the shape selects the node the binding names.
  EXPECT_EQ(live_data.FindDataSourceForNode(kPlantNodeId), kBinding);
}

}  // namespace
