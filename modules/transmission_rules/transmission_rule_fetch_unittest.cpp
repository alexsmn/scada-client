#include "modules/transmission_rules/transmission_rule_fetch.h"

#include "model/devices_node_ids.h"
#include "node_service/test/fake_node_service.h"

#include <boost/asio/io_context.hpp>
#include <gtest/gtest.h>

namespace {

// A rule reads in two hops and a selection makes neither resident, so the
// inspector rendered "— → 0" for every rule an operator selected. What the
// fetch asks for is the contract, so that is what is asserted.
class FetchTransmissionRuleTest : public ::testing::Test {
 protected:
  static constexpr scada::NodeId kRule{733, 12};
  static constexpr scada::NodeId kRuleType{900, 12};
  static constexpr scada::NodeId kBaseType{901, 12};
  static constexpr scada::NodeId kEndpoint{702, 12};
  static constexpr scada::NodeId kSource{723, 12};

  void SetUp() override {
    nodes_.Add(scada::NodeState{.node_id = kBaseType,
                                .node_class = scada::NodeClass::ObjectType});
    nodes_.Add(scada::NodeState{.node_id = kRuleType,
                                .node_class = scada::NodeClass::ObjectType,
                                .supertype_id = kBaseType});
    nodes_.Add(scada::NodeState{.node_id = kEndpoint,
                                .node_class = scada::NodeClass::Object,
                                .type_definition_id = kRuleType});
    nodes_.Add(scada::NodeState{.node_id = kSource,
                                .node_class = scada::NodeClass::Variable,
                                .type_definition_id = kRuleType});
  }

  NodeRef AddRule(scada::NodeId source_id) {
    return nodes_.Add(scada::NodeState{
        .node_id = kRule,
        .node_class = scada::NodeClass::Object,
        .type_definition_id = kRuleType,
        .parent_id = kEndpoint,
        .reference_type_id = scada::id::Organizes,
        .properties = {
            {scada::devices::id::TransmissionItemType_SourceNode, source_id},
            {scada::devices::id::TransmissionItemType_Address,
             static_cast<scada::Int32>(2001)}}});
  }

  void Run(NodeRef rule) {
    RunAwaitable(io_, [rule]() -> Awaitable<void> {
      co_await FetchTransmissionRule(rule);
    });
  }

  boost::asio::io_context io_;
  FakeNodeService nodes_;
};

TEST_F(FetchTransmissionRuleTest, FetchesBothHopsAndTheEndpoint) {
  const NodeRef rule = AddRule(kSource);

  Run(rule);

  EXPECT_EQ(nodes_.fetch_requests(kRule),
            std::vector{NodeFetchStatus::NodeAndChildren});
  // The whole type chain: Address and SourceNode are declared on the
  // TransmissionItemType supertype, so stopping at the immediate type leaves
  // the subscript unable to resolve either declaration.
  EXPECT_EQ(nodes_.fetch_requests(kRuleType),
            std::vector{NodeFetchStatus::NodeAndChildren});
  EXPECT_EQ(nodes_.fetch_requests(kBaseType),
            std::vector{NodeFetchStatus::NodeAndChildren});

  // Each property's own value — SourceNode's especially, since the second hop
  // cannot start until that NodeId can be read.
  for (const scada::NodeId& property_id :
       {scada::devices::id::TransmissionItemType_SourceNode,
        scada::devices::id::TransmissionItemType_Address}) {
    const NodeRef property = rule[property_id];
    ASSERT_TRUE(property) << property_id.ToString();
    EXPECT_EQ(nodes_.fetch_requests(property.node_id()),
              std::vector{NodeFetchStatus::NodeOnly})
        << property_id.ToString();
  }

  // The endpoint the rule hangs under, for its name.
  EXPECT_EQ(nodes_.fetch_requests(kEndpoint),
            std::vector{NodeFetchStatus::NodeOnly});

  // The second hop: a peer node named by a NodeId value, not a child, so
  // nothing above reaches it.
  EXPECT_EQ(nodes_.fetch_requests(kSource),
            std::vector{NodeFetchStatus::NodeOnly});
}

// A rule whose SourceNode is unset is an ordinary half-configured rule, not a
// failure: the second hop is skipped and the first still completes.
TEST_F(FetchTransmissionRuleTest, SkipsTheSecondHopWithoutASource) {
  const NodeRef rule = AddRule(scada::NodeId{});

  Run(rule);

  EXPECT_EQ(nodes_.fetch_requests(kRule),
            std::vector{NodeFetchStatus::NodeAndChildren});
  EXPECT_TRUE(nodes_.fetch_requests(kSource).empty());
}

TEST_F(FetchTransmissionRuleTest, NullRuleCompletes) {
  Run(NodeRef{});
}

}  // namespace
