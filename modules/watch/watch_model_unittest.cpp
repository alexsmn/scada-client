#include "modules/watch/watch_model.h"

#include "aui/severity_colors.h"
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

  // A frame identified only by its link-layer format, for the kind filter.
  void DeliverFormat(std::int64_t seconds, std::string format) {
    scada::DeviceFrameEvent event;
    event.base = MakeEvent(seconds, u"frame");
    event.frame.direction = scada::DeviceFrame::kInbound;
    event.frame.format = std::move(format);
    event_source_.DeliverFrame(event);
  }

  void DeliverWithSeverity(std::int64_t seconds,
                           std::u16string message,
                           scada::UInt32 severity) {
    scada::Event event = MakeEvent(seconds, std::move(message));
    event.severity = severity;
    event_source_.Deliver(event);
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
  DeliverFrame(2, u"decoded ASDU", scada::DeviceFrame::kInbound,
               /*type_id=*/13);

  ASSERT_EQ(model_.GetRowCount(), 2);
  EXPECT_EQ(model_.GetCellText(0, 7), u"U");
  EXPECT_EQ(model_.GetCellText(0, 8), u"");
  // No APCI was parsed for the decoded row, so no format either.
  EXPECT_EQ(model_.GetCellText(1, 7), u"");
  EXPECT_EQ(model_.GetCellText(1, 8), u"");
}

// The kind filter, from the trace mockup's All / I-format / S+U segments.
TEST_F(WatchModelTest, FiltersByFrameKind) {
  DeliverFormat(1, "I");
  DeliverFormat(2, "S");
  DeliverFormat(3, "U");

  model_.SetFilter({.kind = WatchFilter::Kind::kInformation});
  ASSERT_EQ(model_.GetRowCount(), 1);
  EXPECT_EQ(model_.GetCellText(0, 7), u"I");

  // S and U are one choice: neither carries data, and both answer the same
  // question about a link that is not moving.
  model_.SetFilter({.kind = WatchFilter::Kind::kSupervisoryAndUnnumbered});
  ASSERT_EQ(model_.GetRowCount(), 2);
  EXPECT_EQ(model_.GetCellText(0, 7), u"S");
  EXPECT_EQ(model_.GetCellText(1, 7), u"U");

  model_.SetFilter({});
  EXPECT_EQ(model_.GetRowCount(), 3);
}

// A row with no APCI format is not a frame of any format, so no format filter
// admits it — including the legacy marker rows, which would otherwise slip
// through a filter that only tested `frame`.
TEST_F(WatchModelTest, FrameKindFilterExcludesRowsWithoutAFormat) {
  DeliverFormat(1, "I");
  Deliver(2, u"#RX: legacy marker line");
  DeliverFrame(3, u"decoded ASDU", scada::DeviceFrame::kInbound,
               /*type_id=*/13);

  model_.SetFilter({.kind = WatchFilter::Kind::kInformation});
  ASSERT_EQ(model_.GetRowCount(), 1);
  EXPECT_EQ(model_.GetCellText(0, 7), u"I");

  model_.SetFilter({.kind = WatchFilter::Kind::kSupervisoryAndUnnumbered});
  EXPECT_EQ(model_.GetRowCount(), 0);
}

TEST_F(WatchModelTest, FiltersToErrorsOnly) {
  DeliverWithSeverity(1, u"routine", scada::kSeverityNormal);
  DeliverWithSeverity(2, u"t1 timeout", scada::kSeverityWarning);
  DeliverWithSeverity(3, u"link down", scada::kSeverityCritical);

  model_.SetFilter({.errors_only = true});
  ASSERT_EQ(model_.GetRowCount(), 2);
  EXPECT_EQ(model_.GetCellText(0, 2), u"t1 timeout");
  EXPECT_EQ(model_.GetCellText(1, 2), u"link down");
}

// The text filter matches what the operator can see, which is the decoded
// columns — not just the message. Hunting one IOA is the motivating case.
TEST_F(WatchModelTest, TextFilterMatchesTheDecodedColumns) {
  DeliverFrame(1, u"first", scada::DeviceFrame::kInbound, /*type_id=*/13,
               /*cause=*/1, /*object_address=*/4002);
  DeliverFrame(2, u"second", scada::DeviceFrame::kInbound, /*type_id=*/45,
               /*cause=*/6, /*object_address=*/6001);

  model_.SetFilter({.text = u"4002"});
  ASSERT_EQ(model_.GetRowCount(), 1);
  EXPECT_EQ(model_.GetCellText(0, 2), u"first");

  // Type id, from a different column.
  model_.SetFilter({.text = u"45"});
  ASSERT_EQ(model_.GetRowCount(), 1);
  EXPECT_EQ(model_.GetCellText(0, 2), u"second");

  // And the message itself.
  model_.SetFilter({.text = u"FIRST"});
  ASSERT_EQ(model_.GetRowCount(), 1);
  EXPECT_EQ(model_.GetCellText(0, 2), u"first");
}

// The filters compose, and compose with the mode.
TEST_F(WatchModelTest, FiltersComposeWithEachOtherAndWithTheMode) {
  DeliverWithSeverity(1, u"#RX: I-format-ish noise", scada::kSeverityWarning);
  scada::DeviceFrameEvent good;
  good.base = MakeEvent(2, u"healthy");
  good.base.severity = scada::kSeverityNormal;
  good.frame = {.direction = scada::DeviceFrame::kInbound, .format = "I"};
  event_source_.DeliverFrame(good);
  scada::DeviceFrameEvent bad;
  bad.base = MakeEvent(3, u"stalled");
  bad.base.severity = scada::kSeverityCritical;
  bad.frame = {.direction = scada::DeviceFrame::kInbound, .format = "I"};
  event_source_.DeliverFrame(bad);

  model_.SetMode(WatchMode::kFrameTrace);
  model_.SetFilter(
      {.kind = WatchFilter::Kind::kInformation, .errors_only = true});

  ASSERT_EQ(model_.GetRowCount(), 1);
  EXPECT_EQ(model_.GetCellText(0, 2), u"stalled");
}

// Filtering hides rows; it never discards them.
TEST_F(WatchModelTest, ClearingTheFilterRestoresEveryRow) {
  DeliverFormat(1, "I");
  DeliverFormat(2, "S");
  model_.SetFilter({.text = u"nothing matches this"});
  ASSERT_EQ(model_.GetRowCount(), 0);

  model_.SetFilter({});
  EXPECT_EQ(model_.GetRowCount(), 2);
}

// Rows arriving while a filter is active are placed by the same predicate,
// which is where an index-bookkeeping bug would show.
TEST_F(WatchModelTest, RowsArrivingUnderAFilterAreFiltered) {
  model_.SetFilter({.kind = WatchFilter::Kind::kInformation});
  DeliverFormat(1, "I");
  DeliverFormat(2, "S");
  DeliverFormat(3, "I");

  ASSERT_EQ(model_.GetRowCount(), 2);
  EXPECT_EQ(model_.GetCellText(0, 7), u"I");
  EXPECT_EQ(model_.GetCellText(1, 7), u"I");
}

// --- Severity row colours (backlog 772) -------------------------------------

// The device log bands its rows by severity, and both halves of the colour --
// fill AND text -- come from `aui/severity_colors.h`, so the row follows the
// active appearance. The model used to paint two literal `Rgba` fills and set
// no text colour: the fills were theme-independent while the text was not, so
// under the dark appearance the theme's near-white default landed on pale
// yellow and the warning rows were close to invisible. That is what an
// operator on a dark desktop got, and on a frame trace the rows hardest to
// read were the timeouts and the lost links.
TEST_F(WatchModelTest, SeverityRowsTakeBothColoursFromTheSharedResolver) {
  DeliverWithSeverity(1, u"t1 timeout", scada::kSeverityWarning);
  DeliverWithSeverity(2, u"link down", scada::kSeverityCritical);

  const scada::aui::EventRowColors warning =
      scada::aui::EventRowColorsFor(scada::aui::EventBackground::kWarning);
  const scada::aui::EventRowColors critical =
      scada::aui::EventRowColorsFor(scada::aui::EventBackground::kCritical);

  scada::aui::TableCell warning_cell{.row = 0, .column_id = 2};
  model_.GetCell(warning_cell);
  EXPECT_EQ(warning_cell.cell_color, warning.background);
  EXPECT_EQ(warning_cell.text_color, warning.text);

  scada::aui::TableCell critical_cell{.row = 1, .column_id = 2};
  model_.GetCell(critical_cell);
  EXPECT_EQ(critical_cell.cell_color, critical.background);
  EXPECT_EQ(critical_cell.text_color, critical.text);
}

// The same row under the other appearance must come back different. This is
// the assertion the literals could never satisfy: they were the same two
// values in every theme, which is exactly why the dark rows were unreadable.
TEST_F(WatchModelTest, SeverityRowColoursFollowTheActiveAppearance) {
  DeliverWithSeverity(1, u"t1 timeout", scada::kSeverityWarning);

  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kDark);
  scada::aui::TableCell dark{.row = 0, .column_id = 2};
  model_.GetCell(dark);

  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kLight);
  scada::aui::TableCell light{.row = 0, .column_id = 2};
  model_.GetCell(light);

  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kDark);

  EXPECT_NE(dark.text_color, light.text_color);
}

// A routine row is not an alarm, so it keeps the palette's own colours -- a
// calm surface draws the eye only to abnormal conditions.
TEST_F(WatchModelTest, ARoutineRowIsLeftUncoloured) {
  DeliverWithSeverity(1, u"routine", scada::kSeverityNormal);

  scada::aui::TableCell cell{.row = 0, .column_id = 2};
  model_.GetCell(cell);

  EXPECT_EQ(cell.cell_color,
            scada::aui::Color{scada::aui::ColorCode::Transparent});
  EXPECT_EQ(cell.text_color,
            scada::aui::Color{scada::aui::ColorCode::Transparent});
}

}  // namespace
