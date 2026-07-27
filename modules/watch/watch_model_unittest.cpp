#include "modules/watch/watch_model.h"

#include "node_service/static/static_node_service.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

constexpr scada::NodeId kDeviceTypeId{8001, 1};
constexpr scada::NodeId kDeviceId{8002, 1};

// Captures the delegate WatchModel hands it, so the test can push events down
// the same path the real source uses instead of reaching into the model.
class CapturingEventSource : public WatchEventSource {
 public:
  void Start(const scada::NodeId&,
             const scada::TimeRange&,
             Delegate& delegate) override {
    delegate_ = &delegate;
  }

  void Deliver(const scada::Event& event) { delegate_->OnEvent(event); }

 private:
  Delegate* delegate_ = nullptr;
};

scada::Event MakeEvent(std::int64_t seconds, std::u16string message) {
  return scada::Event{.time = scada::Time{} + std::chrono::seconds(seconds),
                      .message = std::move(message)};
}

class WatchModelTest : public testing::Test {
 protected:
  WatchModelTest() {
    node_service_.Add(
        {.node_id = kDeviceTypeId, .node_class = scada::NodeClass::ObjectType});
    node_service_.Add({.node_id = kDeviceId,
                       .node_class = scada::NodeClass::Object,
                       .type_definition_id = kDeviceTypeId});
    // Binds the source's delegate.
    model_.SetDevice(node_service_.GetNode(kDeviceId));
  }

  void Deliver(std::int64_t seconds, std::u16string message) {
    event_source_.Deliver(MakeEvent(seconds, std::move(message)));
  }

  StaticNodeService node_service_;
  CapturingEventSource event_source_;
  WatchModel model_{WatchModelContext{.node_service_ = node_service_,
                                      .event_source_ = event_source_}};
};

// The whole point of the mode: the frame trace shows the lines the drivers
// marked as protocol traffic and hides the operational chatter around them.
TEST_F(WatchModelTest, FrameTraceShowsOnlyMarkedTraffic) {
  Deliver(1, u"#RX: 68 0C FA 0F (14 bytes)");
  Deliver(2, u"Transmission completed");
  Deliver(3, u"$TX: Send write confirmation [addr=6001]");
  Deliver(4, u"Message is too short");

  EXPECT_EQ(model_.GetRowCount(), 4);

  model_.SetMode(WatchMode::kFrameTrace);
  EXPECT_EQ(model_.GetRowCount(), 2);
  EXPECT_EQ(model_.GetCellText(0, 2), u"RX: 68 0C FA 0F (14 bytes)");
  EXPECT_EQ(model_.GetCellText(0, 3), u"RX");
  EXPECT_EQ(model_.GetCellText(1, 2),
            u"TX: Send write confirmation [addr=6001]");
  EXPECT_EQ(model_.GetCellText(1, 3), u"TX");

  // Switching back restores every line; the mode filters, it does not discard.
  model_.SetMode(WatchMode::kLog);
  EXPECT_EQ(model_.GetRowCount(), 4);
}

// Events arriving while the trace is active must land in the right place —
// this is where an index-shifting bug would show up.
TEST_F(WatchModelTest, TrafficArrivingInFrameTraceModeIsAppended) {
  model_.SetMode(WatchMode::kFrameTrace);
  Deliver(1, u"#RX: first");
  Deliver(2, u"noise");
  Deliver(3, u"$TX: second");

  ASSERT_EQ(model_.GetRowCount(), 2);
  EXPECT_EQ(model_.GetCellText(0, 2), u"RX: first");
  EXPECT_EQ(model_.GetCellText(1, 2), u"TX: second");
}

// Out-of-order delivery: the model keeps events sorted by time, so the visible
// index must be recomputed rather than assumed to be an append.
TEST_F(WatchModelTest, OutOfOrderTrafficKeepsTimeOrder) {
  model_.SetMode(WatchMode::kFrameTrace);
  Deliver(3, u"#RX: later");
  Deliver(2, u"noise");
  Deliver(1, u"$TX: earlier");

  ASSERT_EQ(model_.GetRowCount(), 2);
  EXPECT_EQ(model_.GetCellText(0, 2), u"TX: earlier");
  EXPECT_EQ(model_.GetCellText(1, 2), u"RX: later");
}

// The direction column is populated in the log mode too — it is what tells an
// operator which lines are traffic before they switch.
TEST_F(WatchModelTest, DirectionIsShownInLogMode) {
  Deliver(1, u"#RX: framed");
  Deliver(2, u"plain line");

  ASSERT_EQ(model_.GetRowCount(), 2);
  EXPECT_EQ(model_.GetCellText(0, 3), u"RX");
  EXPECT_EQ(model_.GetCellText(1, 3), u"");
  // The marker never reaches the message column.
  EXPECT_EQ(model_.GetCellText(0, 2), u"RX: framed");
  EXPECT_EQ(model_.GetCellText(1, 2), u"plain line");
}

TEST_F(WatchModelTest, ClearEmptiesBothModes) {
  Deliver(1, u"#RX: framed");
  Deliver(2, u"plain line");
  model_.SetMode(WatchMode::kFrameTrace);
  ASSERT_EQ(model_.GetRowCount(), 1);

  model_.Clear();
  EXPECT_EQ(model_.GetRowCount(), 0);
  model_.SetMode(WatchMode::kLog);
  EXPECT_EQ(model_.GetRowCount(), 0);
}

}  // namespace
