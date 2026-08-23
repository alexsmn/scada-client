#include "modules/inspector/limit_band.h"

#include "model/data_items_node_ids.h"
#include "node_service/test/fake_node_service.h"

#include <boost/asio/io_context.hpp>
#include <gtest/gtest.h>

namespace {

constexpr LimitValues kFullBands{.lolo = 10.0,
                                 .lo = 20.0,
                                 .hi = 80.0,
                                 .hihi = 90.0};

TEST(LimitBandTest, ValueInsideTheBandsIsNormal) {
  EXPECT_EQ(LimitBandFor(50.0, kFullBands), LimitBand::kNormal);
}

TEST(LimitBandTest, ClassifiesEachBand) {
  EXPECT_EQ(LimitBandFor(85.0, kFullBands), LimitBand::kHi);
  EXPECT_EQ(LimitBandFor(95.0, kFullBands), LimitBand::kHiHi);
  EXPECT_EQ(LimitBandFor(15.0, kFullBands), LimitBand::kLo);
  EXPECT_EQ(LimitBandFor(5.0, kFullBands), LimitBand::kLoLo);
}

// A value exactly on a limit breaches it: the limit is the threshold the
// process is meant to stay inside.
TEST(LimitBandTest, ValueOnTheLimitBreachesIt) {
  EXPECT_EQ(LimitBandFor(80.0, kFullBands), LimitBand::kHi);
  EXPECT_EQ(LimitBandFor(90.0, kFullBands), LimitBand::kHiHi);
  EXPECT_EQ(LimitBandFor(20.0, kFullBands), LimitBand::kLo);
  EXPECT_EQ(LimitBandFor(10.0, kFullBands), LimitBand::kLoLo);
}

// Bands the node does not configure are never reported, so a node carrying
// only warning limits cannot claim an alarm-level breach.
TEST(LimitBandTest, UnconfiguredBandsAreNeverReported) {
  constexpr LimitValues warning_only{.lo = 20.0, .hi = 80.0};
  EXPECT_EQ(LimitBandFor(1000.0, warning_only), LimitBand::kHi);
  EXPECT_EQ(LimitBandFor(-1000.0, warning_only), LimitBand::kLo);

  constexpr LimitValues none;
  EXPECT_TRUE(none.empty());
  EXPECT_EQ(LimitBandFor(1000.0, none), LimitBand::kNormal);
}

// A misconfigured node whose Hi sits above its HiHi still reports the more
// severe breach rather than the narrower one.
TEST(LimitBandTest, OverlappingBandsReportTheMoreSevere) {
  constexpr LimitValues inverted{.hi = 95.0, .hihi = 90.0};
  EXPECT_EQ(LimitBandFor(96.0, inverted), LimitBand::kHiHi);
}

// The bands are only readable once three separate things are resident, and a
// selection makes none of them so — which is why the Inspector's Measurements
// block was hidden for every node an operator selected until this fetch
// existed. The requests are the contract, so they are what is asserted.
class FetchLimitBandsTest : public ::testing::Test {
 protected:
  static constexpr scada::NodeId kItem{700, 12};
  static constexpr scada::NodeId kItemType{800, 12};
  static constexpr scada::NodeId kBaseType{801, 12};

  void Run(NodeRef item) {
    RunAwaitable(
        io_, [item]() -> Awaitable<void> { co_await FetchLimitBands(item); });
  }

  boost::asio::io_context io_;
  FakeNodeService nodes_;
};

TEST_F(FetchLimitBandsTest, FetchesTheItemItsTypeChainAndEveryBand) {
  nodes_.Add(scada::NodeState{.node_id = kBaseType,
                              .node_class = scada::NodeClass::VariableType});
  nodes_.Add(scada::NodeState{.node_id = kItemType,
                              .node_class = scada::NodeClass::VariableType,
                              .supertype_id = kBaseType});
  const NodeRef item = nodes_.Add(scada::NodeState{
      .node_id = kItem,
      .node_class = scada::NodeClass::Variable,
      .type_definition_id = kItemType,
      .properties = {{scada::data_items::id::AnalogItemType_LimitHiHi, 90.0},
                     {scada::data_items::id::AnalogItemType_LimitHi, 80.0},
                     {scada::data_items::id::AnalogItemType_LimitLo, 20.0},
                     {scada::data_items::id::AnalogItemType_LimitLoLo, 10.0}}});

  Run(item);

  // The item and both types: the subscript resolves an aggregate declaration
  // against the type, and the declaration can sit on a supertype — fetching the
  // immediate type alone leaves an inherited band unreadable.
  EXPECT_EQ(nodes_.fetch_requests(kItem),
            std::vector{NodeFetchStatus::NodeAndChildren});
  EXPECT_EQ(nodes_.fetch_requests(kItemType),
            std::vector{NodeFetchStatus::NodeAndChildren});
  EXPECT_EQ(nodes_.fetch_requests(kBaseType),
            std::vector{NodeFetchStatus::NodeAndChildren});

  // Each band node's own value, without which the band reads back empty and the
  // node looks unconfigured.
  for (const scada::NodeId& band :
       {scada::data_items::id::AnalogItemType_LimitHiHi,
        scada::data_items::id::AnalogItemType_LimitHi,
        scada::data_items::id::AnalogItemType_LimitLo,
        scada::data_items::id::AnalogItemType_LimitLoLo}) {
    const NodeRef property = item[band];
    ASSERT_TRUE(property) << band.ToString();
    EXPECT_EQ(nodes_.fetch_requests(property.node_id()),
              std::vector{NodeFetchStatus::NodeOnly})
        << band.ToString();
  }
}

// A node configuring fewer bands is an ordinary node, not a failure: the
// missing ones resolve to null refs and are skipped.
TEST_F(FetchLimitBandsTest, SkipsBandsTheItemDoesNotConfigure) {
  nodes_.Add(scada::NodeState{.node_id = kItemType,
                              .node_class = scada::NodeClass::VariableType});
  const NodeRef item = nodes_.Add(scada::NodeState{
      .node_id = kItem,
      .node_class = scada::NodeClass::Variable,
      .type_definition_id = kItemType,
      .properties = {{scada::data_items::id::AnalogItemType_LimitHi, 80.0}}});

  Run(item);

  EXPECT_TRUE(item[scada::data_items::id::AnalogItemType_LimitHi]);
  EXPECT_FALSE(item[scada::data_items::id::AnalogItemType_LimitLoLo]);
}

// A null item is what an event or expression-row selection hands over; asking
// for its bands must be a no-op rather than a crash.
TEST_F(FetchLimitBandsTest, NullItemCompletes) {
  Run(NodeRef{});
}

}  // namespace
