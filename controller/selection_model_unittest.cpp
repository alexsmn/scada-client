#include "controller/selection_model.h"

#include "node_service/test/fake_node_service.h"
#include "scada/event.h"
#include "timed_data/timed_data_service_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

class SelectionModelTest : public testing::Test {
 protected:
  SelectionModelTest() {
    node_service_.Add(scada::NodeState{.node_id = kNode});
    selection_.change_handler = [this] { ++changes_; };
  }

  static constexpr scada::NodeId kNode{1, 1};

  FakeNodeService node_service_;
  testing::NiceMock<MockTimedDataService> timed_data_service_;
  SelectionModel selection_{{timed_data_service_}};
  int changes_ = 0;
};

// The regression: SelectEvent keeps the event's source as node_, and
// SelectNode returned early on `node_ == node` before looking at the
// selection type — so clicking an event's source in the Explorer left the
// selection on the event and fired no change.
TEST_F(SelectionModelTest, SelectNodeSwitchesAwayFromAnEventWithThatSource) {
  const NodeRef source = node_service_.GetNode(kNode);
  selection_.SelectEvent(scada::Event{}, source);
  ASSERT_TRUE(selection_.event().has_value());
  ASSERT_EQ(changes_, 1);

  selection_.SelectNode(source);

  EXPECT_FALSE(selection_.event().has_value());
  EXPECT_EQ(selection_.node(), source);
  EXPECT_EQ(changes_, 2);
}

// The no-op that the early return exists for is unchanged: re-selecting the
// node that is already the node selection fires nothing.
TEST_F(SelectionModelTest, ReselectingTheSelectedNodeIsANoOp) {
  const NodeRef node = node_service_.GetNode(kNode);
  selection_.SelectNode(node);
  ASSERT_EQ(changes_, 1);

  selection_.SelectNode(node);

  EXPECT_EQ(changes_, 1);
}

}  // namespace
