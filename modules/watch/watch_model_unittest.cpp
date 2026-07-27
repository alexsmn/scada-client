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

  void DeliverFrame(const scada::DeviceFrameEvent& event) {
    delegate_->OnDeviceFrame(event);
  }

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

  // A server that reports structured frame data.
  void DeliverFrame(std::int64_t seconds,
                    std::u16string message,
                    scada::Int32 direction,
                    scada::Int32 type_id = 0,
                    scada::Int32 cause = 0,
                    scada::Int32 object_address = 0) {
    scada::DeviceFrameEvent event;
    event.base = MakeEvent(seconds, std::move(message));
    event.frame.direction = direction;
    event.frame.type_id = type_id;
    event.frame.cause = cause;
    event.frame.object_address = object_address;
    event_source_.DeliverFrame(event);
  }

  void DeliverApci(std::int64_t seconds,
                   std::string format,
                   scada::Int32 send_sequence,
                   scada::Int32 receive_sequence) {
    scada::DeviceFrameEvent event;
    event.base = MakeEvent(seconds, u"raw frame");
    event.frame.direction = scada::DeviceFrame::kInbound;
    event.frame.format = std::move(format);
    event.frame.send_sequence = send_sequence;
    event.frame.receive_sequence = receive_sequence;
    event_source_.DeliverFrame(event);
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


// The structured path: direction and the decoded columns come from the frame,
// not from parsing the message back apart.
TEST_F(WatchModelTest, StructuredFramesPopulateTheDecodedColumns) {
  DeliverFrame(1, u"M_ME_NC_1 received", scada::DeviceFrame::kInbound,
               /*type_id=*/13, /*cause=*/1, /*object_address=*/4002);

  ASSERT_EQ(model_.GetRowCount(), 1);
  EXPECT_EQ(model_.GetCellText(0, 3), u"RX");
  EXPECT_EQ(model_.GetCellText(0, 4), u"13");
  EXPECT_EQ(model_.GetCellText(0, 5), u"1");
  EXPECT_EQ(model_.GetCellText(0, 6), u"4002");
  // The message needs no marker now, and is shown untouched.
  EXPECT_EQ(model_.GetCellText(0, 2), u"M_ME_NC_1 received");
}

// A structured frame is traffic by construction, with no marker to parse.
TEST_F(WatchModelTest, FrameTraceShowsStructuredFramesWithoutAMarker) {
  DeliverFrame(1, u"unmarked but structured", scada::DeviceFrame::kOutbound);
  Deliver(2, u"plain log line");

  model_.SetMode(WatchMode::kFrameTrace);
  ASSERT_EQ(model_.GetRowCount(), 1);
  EXPECT_EQ(model_.GetCellText(0, 2), u"unmarked but structured");
  EXPECT_EQ(model_.GetCellText(0, 3), u"TX");
}

// Mixed-version deployments: a server older than DeviceFrameEventType still
// sends prose with #/$ markers, and must keep working.
TEST_F(WatchModelTest, FallsBackToTheMessageMarkerWithoutStructuredData) {
  Deliver(1, u"#RX: 68 0C");
  DeliverFrame(2, u"structured", scada::DeviceFrame::kInbound, 13);

  model_.SetMode(WatchMode::kFrameTrace);
  ASSERT_EQ(model_.GetRowCount(), 2);
  EXPECT_EQ(model_.GetCellText(0, 3), u"RX");
  // The legacy row has no decoded fields to show — they stay blank rather than
  // being guessed out of the text.
  EXPECT_EQ(model_.GetCellText(0, 4), u"");
  EXPECT_EQ(model_.GetCellText(1, 4), u"13");
}

// Zero is "not applicable", not a value: IOA 0 is not a valid object address
// and type/cause 0 are unused, so showing "0" would invent data.
TEST_F(WatchModelTest, UnsetFrameFieldsRenderBlank) {
  DeliverFrame(1, u"raw frame", scada::DeviceFrame::kInbound);

  ASSERT_EQ(model_.GetRowCount(), 1);
  EXPECT_EQ(model_.GetCellText(0, 4), u"");
  EXPECT_EQ(model_.GetCellText(0, 5), u"");
  EXPECT_EQ(model_.GetCellText(0, 6), u"");
  EXPECT_EQ(model_.GetCellText(0, 3), u"RX");
}


// The APCI columns. N(S)/N(R) share a cell because that is how the standard
// names them and how an engineer reads a stalled send window.
TEST_F(WatchModelTest, ShowsApciFormatAndSequenceNumbers) {
  DeliverApci(1, "I", 2045, 1602);

  ASSERT_EQ(model_.GetRowCount(), 1);
  EXPECT_EQ(model_.GetCellText(0, 7), u"I");
  EXPECT_EQ(model_.GetCellText(0, 8), u"2045/1602");
}

// S-format acknowledges without sending, so N(S) does not exist. Showing "0/n"
// would claim a sequence number the frame never carried — zero is itself a
// valid N(S).
TEST_F(WatchModelTest, SupervisoryFramesShowOnlyTheReceiveSequence) {
  DeliverApci(1, "S", /*send=*/0, /*receive=*/1602);

  ASSERT_EQ(model_.GetRowCount(), 1);
  EXPECT_EQ(model_.GetCellText(0, 7), u"S");
  EXPECT_EQ(model_.GetCellText(0, 8), u"\u2014/1602");
}

// U-format is unnumbered, and a decoded-ASDU row never saw the wire header at
// all: both leave the sequence cell blank rather than inventing numbers.
TEST_F(WatchModelTest, UnnumberedAndDecodedRowsHaveNoSequenceNumbers) {
  DeliverApci(1, "U", 0, 0);
  DeliverFrame(2, u"decoded ASDU", scada::DeviceFrame::kInbound, /*type_id=*/13);

  ASSERT_EQ(model_.GetRowCount(), 2);
  EXPECT_EQ(model_.GetCellText(0, 7), u"U");
  EXPECT_EQ(model_.GetCellText(0, 8), u"");
  // No APCI was parsed for the decoded row, so no format either.
  EXPECT_EQ(model_.GetCellText(1, 7), u"");
  EXPECT_EQ(model_.GetCellText(1, 8), u"");
}

}  // namespace
