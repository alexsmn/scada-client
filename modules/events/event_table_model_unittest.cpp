#include "events/event_table_model.h"

#include "base/test/test_executor.h"
#include "events/alarm_flood.h"
#include "events/current_event_model.h"
#include "events/event_grouping.h"
#include "events/event_severity.h"
#include "events/expanded_event_model.h"
#include "events/historical_event_model.h"
#include "events/local_event_model.h"
#include "events/local_events.h"
#include "events/node_event_provider_mock.h"
#include "model/data_items_node_ids.h"
#include "model/namespaces.h"
#include "model/node_id_util.h"
#include "node_service/node_service_mock.h"
#include "node_service/static/static_node_service.h"
#include "scada/history_service_mock.h"
#include "scada/standard_node_ids.h"

#include "base/utf_convert.h"

#include <gmock/gmock.h>
#include <set>

using namespace testing;

namespace {

struct TestNodeGenerator {
  scada::NodeId node_id(int index) const {
    return {static_cast<scada::NumericId>(index), scada::NamespaceIndexes::TIT};
  }

  scada::NodeId type_definition_id() const {
    return scada::data_items::id::AnalogItemType;
  }

  scada::LocalizedText display_name(int index) const {
    return UtfConvert<char16_t>(std::format("Event {}", index + 1));
  }

  const int count = 3;
};

NodeEventProvider::EventContainer GenerateEvents(const TestNodeGenerator& nodes,
                                                 int start,
                                                 int count) {
  NodeEventProvider::EventContainer events;
  for (int i = 0; i < count; ++i) {
    int index = i + start;
    scada::EventId event_id = static_cast<scada::EventId>(index + 1);
    scada::Event event{
        .event_id = event_id,
        .source_node_id = nodes.node_id(index % nodes.count),
        .message = UtfConvert<char16_t>(std::format("Event {}", index + 1))};
    events.try_emplace(event_id, std::move(event));
  }
  return events;
}

}  // namespace

class EventTableModelTest : public Test {
 protected:
  EventTableModelTest();
  ~EventTableModelTest();

  void Init();
  void ValidateEvents();

  TestExecutor executor_;

  StaticNodeService node_service_;

  // TODO: Use an interface instead of the `CurrentEventModel` implementation.
  StrictMock<MockNodeEventProvider> node_event_provider_;
  std::optional<CurrentEventModel> current_event_model_;

  // TODO: Use an interface instead of the `HistoricalEventModel`
  // implementation.
  StrictMock<scada::MockHistoryService> history_service_;
  HistoricalEventModel historical_event_model_{executor_, history_service_};

  // TODO: Use an interface instead of the `LocalEventModel`
  // implementation.
  LocalEvents local_events_;
  LocalEventModel local_event_model_{local_events_};

  std::optional<EventTableModel> event_table_model_;

  EventObserver* event_observer_ = nullptr;

  TestNodeGenerator test_nodes_{.count = 3};

  // `EventTableModel` requires stable event pointers. The container type is
  // mandated by `NodeEventProvider::unacked_events()`.
  NodeEventProvider::EventContainer test_events_ =
      GenerateEvents(test_nodes_, 0, 100);
};

EventTableModelTest::EventTableModelTest() {
  for (int i = 0; i < test_nodes_.count; ++i) {
    node_service_.Add(
        {.node_id = test_nodes_.node_id(i),
         .type_definition_id = test_nodes_.type_definition_id(),
         .attributes = {
             .browse_name = NodeIdToScadaString(test_nodes_.node_id(i)),
             .display_name = test_nodes_.display_name(i)}});
  }

  ON_CALL(node_event_provider_, unacked_events())
      .WillByDefault(ReturnRef(test_events_));

  EXPECT_CALL(node_event_provider_, AddObserver(_))
      .WillOnce(Invoke([&](EventObserver& obs) { event_observer_ = &obs; }));

  current_event_model_.emplace(node_event_provider_);

  event_table_model_.emplace(
      EventTableModelContext{.executor_ = executor_,
                             .node_service_ = node_service_,
                             .current_event_model_ = *current_event_model_,
                             .historical_event_model_ = historical_event_model_,
                             .local_event_model_ = local_event_model_,
                             .current_events_ = true});
}

EventTableModelTest::~EventTableModelTest() {
  EXPECT_CALL(node_event_provider_, RemoveObserver(_))
      .WillOnce(Assign(&event_observer_, nullptr));
}

void EventTableModelTest::Init() {
  EXPECT_CALL(node_event_provider_, unacked_events());

  event_table_model_->Init(/*range*/ {}, /*filter_items*/ {});

  ASSERT_THAT(event_observer_, NotNull());
}

void EventTableModelTest::ValidateEvents() {
  std::vector<const scada::Event*> actual_events;

  for (int i = 0; i < event_table_model_->GetRowCount(); ++i) {
    actual_events.emplace_back(&event_table_model_->event_at(i));
  }

  std::vector<const scada::Event*> expected_events;
  for (const auto& event :
       test_events_ | std::views::values |
           std::views::filter([](const auto& e) { return !e.acked; })) {
    expected_events.emplace_back(&event);
  }

  EXPECT_EQ(actual_events, expected_events);
}

TEST_F(EventTableModelTest, CurrentEvents_InitialEvents) {
  Init();
  ValidateEvents();
}

TEST_F(EventTableModelTest, CurrentEvents_NewUnackedEvents) {
  Init();

  auto new_events = GenerateEvents(test_nodes_, test_events_.size(), 10);

  std::vector<const scada::Event*> new_event_ptrs;
  for (const auto& [ack_id, event] : new_events) {
    new_event_ptrs.emplace_back(
        &test_events_.try_emplace(ack_id, event).first->second);
  }

  event_observer_->OnEvents(new_event_ptrs);

  ValidateEvents();
}

// The historical journal can be filtered to the actionable (unacknowledged)
// events — the mockup's "Unacknowledged only" control — while off it keeps the
// full history. Built in historical mode with an empty current surface so only
// the historical rows are under test; refilter is driven synchronously through
// the historical model's refilter_now signal (no async history read).
TEST(EventTableModelUnacknowledgedFilterTest,
     HidesAcknowledgedHistoricalEvents) {
  TestExecutor executor;

  StaticNodeService node_service;
  const scada::NodeId node_id{1, scada::NamespaceIndexes::TIT};
  node_service.Add(
      {.node_id = node_id,
       .type_definition_id = scada::data_items::id::AnalogItemType,
       .attributes = {.browse_name = "n1", .display_name = u"N1"}});

  NiceMock<MockNodeEventProvider> node_event_provider;
  NodeEventProvider::EventContainer empty_current;
  ON_CALL(node_event_provider, unacked_events())
      .WillByDefault(ReturnRef(empty_current));
  CurrentEventModel current_event_model{node_event_provider};

  NiceMock<scada::MockHistoryService> history_service;
  HistoricalEventModel historical_event_model{executor, history_service};
  LocalEvents local_events;
  LocalEventModel local_event_model{local_events};

  EventTableModel model{{.executor_ = executor,
                         .node_service_ = node_service,
                         .current_event_model_ = current_event_model,
                         .historical_event_model_ = historical_event_model,
                         .local_event_model_ = local_event_model,
                         .current_events_ = false}};

  historical_event_model.AddEvent({.event_id = 1, .source_node_id = node_id});
  historical_event_model.AddEvent({.event_id = 2, .source_node_id = node_id});
  historical_event_model.AddEvent(
      {.event_id = 3, .source_node_id = node_id, .acked = true});

  // Off: the full history (two unacked + one acked).
  historical_event_model.refilter_now();
  EXPECT_EQ(model.GetRowCount(), 3);

  // On: only the actionable (unacknowledged) events remain.
  model.SetUnacknowledgedOnly(true);
  EXPECT_EQ(model.GetRowCount(), 2);
  for (int row = 0; row < model.GetRowCount(); ++row)
    EXPECT_FALSE(model.event_at(row).acked);

  // Back off: the acknowledged event returns.
  model.SetUnacknowledgedOnly(false);
  EXPECT_EQ(model.GetRowCount(), 3);
}

TEST_F(EventTableModelTest, CurrentEvents_AckEvents) {
  Init();

  const auto ack_time = scada::Now();
  const int ack_count = 10;

  std::vector<const scada::Event*> event_ptrs;
  for (auto& event :
       test_events_ | std::views::values | std::views::take(ack_count)) {
    event.acked = true;
    event.acknowledged_time = ack_time;
    event.acknowledged_user_id = {1, scada::NamespaceIndexes::USER};
    event_ptrs.emplace_back(&event);
  }

  event_observer_->OnEvents(event_ptrs);

  ValidateEvents();
}

// The reshelled journal reads as an alarm surface (UX backlog 2.2): a row
// states its alarm band in words and says outright when it still awaits an
// operator. Both are theme-gated, so the legacy journal is byte-for-byte
// unchanged.
class EventJournalAlarmSurfaceTest : public EventTableModelTest {
 protected:
  void TearDown() override {
    scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kLegacy);
  }

  std::u16string CellText(int column_id, int row = 0) {
    scada::aui::TableCell cell{.row = row, .column_id = column_id};
    event_table_model_->GetCell(cell);
    return cell.text;
  }

  // Row 0's event, seeded before the model reads the event set.
  scada::Event& FirstEvent() { return test_events_.begin()->second; }

  scada::aui::Color CellBackground(int row = 0) {
    scada::aui::TableCell cell{.row = row, .column_id = EventColumnMessage};
    event_table_model_->GetCell(cell);
    return cell.cell_color;
  }

  static scada::aui::Color BackgroundFor(scada::aui::EventBackground kind) {
    return scada::aui::EventRowColorsFor(kind).background;
  }
};

// Under the reshell theme the row colour is the event's *severity*, whether or
// not it has been acknowledged: a pending critical alarm used to paint green
// (the unacknowledged colour won the classification), which reads as "normal"
// and spends saturated colour on something that is not a severity.
// Acknowledgement rides the dot column, the pending cell and the footer.
TEST_F(EventJournalAlarmSurfaceTest,
       ThemedPendingCriticalKeepsItsSeverityColour) {
  FirstEvent().severity = scada::kSeverityCritical;
  FirstEvent().acked = false;
  Init();
  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kDark);

  EXPECT_EQ(CellBackground(),
            BackgroundFor(scada::aui::EventBackground::kCritical));
  EXPECT_NE(CellBackground(),
            BackgroundFor(scada::aui::EventBackground::kUnacknowledged));
}

// Acknowledging does not change the severity, so it does not change the colour.
TEST_F(EventJournalAlarmSurfaceTest,
       ThemedAcknowledgedCriticalKeepsTheSameColour) {
  FirstEvent().severity = scada::kSeverityCritical;
  FirstEvent().acked = true;
  Init();
  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kDark);

  EXPECT_EQ(CellBackground(),
            BackgroundFor(scada::aui::EventBackground::kCritical));
}

// A routine event is not an alarm, so a themed journal leaves it uncoloured
// even while it is pending — a calm surface draws the eye only to alarms
// (principles.md §1).
TEST_F(EventJournalAlarmSurfaceTest, ThemedPendingRoutineEventStaysUncoloured) {
  FirstEvent().severity = scada::kSeverityNormal;
  FirstEvent().acked = false;
  Init();
  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kDark);

  EXPECT_NE(CellBackground(),
            BackgroundFor(scada::aui::EventBackground::kUnacknowledged));
  EXPECT_NE(CellBackground(),
            BackgroundFor(scada::aui::EventBackground::kCritical));
  EXPECT_NE(CellBackground(),
            BackgroundFor(scada::aui::EventBackground::kWarning));
}

// The pending dot must stay readable on the severity-coloured row it marks:
// colouring it by severity too (as it once was) painted a red dot on a red
// critical row, erasing the very cue it exists to provide.
TEST_F(EventJournalAlarmSurfaceTest, ThemedPendingDotContrastsWithItsRow) {
  FirstEvent().severity = scada::kSeverityCritical;
  FirstEvent().acked = false;
  Init();
  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kDark);

  scada::aui::TableCell cell{.row = 0, .column_id = EventColumnUnacked};
  event_table_model_->GetCell(cell);
  EXPECT_EQ(cell.text, u"●");
  EXPECT_NE(cell.text_color, cell.cell_color);
}

// The legacy journal has no dot column, no pending cell and no footer, so its
// green background is the only unacknowledged signal there and is left alone.
TEST_F(EventJournalAlarmSurfaceTest,
       LegacyPendingRowKeepsTheUnacknowledgedColour) {
  FirstEvent().severity = scada::kSeverityCritical;
  FirstEvent().acked = false;
  Init();

  EXPECT_EQ(CellBackground(),
            BackgroundFor(scada::aui::EventBackground::kUnacknowledged));
}

TEST_F(EventJournalAlarmSurfaceTest, LegacySeverityCellIsTheBareNumber) {
  FirstEvent().severity = scada::kSeverityCritical;
  Init();

  EXPECT_EQ(CellText(EventColumnSeverity), u"800");
}

TEST_F(EventJournalAlarmSurfaceTest, ThemedSeverityCellNamesTheAlarmBand) {
  FirstEvent().severity = scada::kSeverityCritical;
  Init();
  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kDark);

  // The band is named as well as numbered, so severity does not depend on the
  // row's colour alone.
  EXPECT_EQ(CellText(EventColumnSeverity),
            events::SeverityLevelLabel(scada::aui::SeverityLevel::kCritical) +
                u" 800");
}

// A routine event has no alarm band, so there is nothing to name — it stays the
// bare number rather than gaining a misleading label.
TEST_F(EventJournalAlarmSurfaceTest, ThemedRoutineSeverityCellStaysTheNumber) {
  FirstEvent().severity = scada::kSeverityNormal;
  Init();
  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kDark);

  EXPECT_EQ(CellText(EventColumnSeverity), u"500");
}

TEST_F(EventJournalAlarmSurfaceTest, ThemedAckCellSaysAnAlarmIsStillPending) {
  Init();
  ASSERT_FALSE(event_table_model_->event_at(0).acked);

  // Legacy: blank, as before.
  EXPECT_EQ(CellText(EventColumnAckTime), u"");

  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kDark);
  EXPECT_FALSE(CellText(EventColumnAckTime).empty());
}

TEST_F(EventJournalAlarmSurfaceTest, AnAcknowledgedRowKeepsShowingItsAckTime) {
  scada::Event& event = FirstEvent();
  event.acked = true;
  event.acknowledged_time = scada::Now();
  Init();
  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kDark);

  const std::u16string text = CellText(EventColumnAckTime);
  EXPECT_FALSE(text.empty());
  EXPECT_EQ(text.find(u"—"), std::u16string::npos);
}

// Severity sorts by the severity, not by the text of its cell — so a themed
// journal that names the band, and a legacy one that shows three-digit
// severities, both order correctly.
TEST_F(EventJournalAlarmSurfaceTest, SeverityColumnSortsNumerically) {
  auto event = test_events_.begin();
  event->second.severity = 100;
  scada::Event& high = event->second;
  scada::Event& low = (++event)->second;
  low.severity = 80;
  Init();

  int high_row = -1, low_row = -1;
  for (int row = 0; row < event_table_model_->GetRowCount(); ++row) {
    if (&event_table_model_->event_at(row) == &high)
      high_row = row;
    else if (&event_table_model_->event_at(row) == &low)
      low_row = row;
  }
  ASSERT_NE(high_row, -1);
  ASSERT_NE(low_row, -1);

  EXPECT_GT(
      event_table_model_->CompareCells(high_row, low_row, EventColumnSeverity),
      0);
}

// Alarm-flood grouping (UX backlog 2.5): while the operator is buried, repeats
// of one alarm collapse into a single counted row instead of a scroll.
class EventFloodGroupingTest : public Test {
 protected:
  EventFloodGroupingTest() {
    node_service_.Add(
        {.node_id = node_id_,
         .type_definition_id = scada::data_items::id::AnalogItemType,
         .attributes = {.browse_name = "n1", .display_name = u"N1"}});
  }

  // Adds `count` occurrences of the same alarm (same source and message), each
  // a second apart so they have distinct times.
  void AddRepeats(std::u16string message, int count, int first_id) {
    for (int i = 0; i < count; ++i) {
      historical_event_model_.AddEvent(
          {.event_id = static_cast<scada::EventId>(first_id + i),
           .time = scada::DateTime{} +
                   std::chrono::seconds(i),
           .source_node_id = node_id_,
           .message = message});
    }
  }

  void Rebuild() { historical_event_model_.refilter_now(); }

  TestExecutor executor_;
  StaticNodeService node_service_;
  const scada::NodeId node_id_{1, scada::NamespaceIndexes::TIT};

  NiceMock<MockNodeEventProvider> node_event_provider_;
  NodeEventProvider::EventContainer empty_current_;
  CurrentEventModel current_event_model_{node_event_provider_};

  NiceMock<scada::MockHistoryService> history_service_;
  HistoricalEventModel historical_event_model_{executor_, history_service_};
  LocalEvents local_events_;
  LocalEventModel local_event_model_{local_events_};

  EventTableModel model_{{.executor_ = executor_,
                          .node_service_ = node_service_,
                          .current_event_model_ = current_event_model_,
                          .historical_event_model_ = historical_event_model_,
                          .local_event_model_ = local_event_model_,
                          .current_events_ = false}};
};

// A locale-qualified message (server-side catalog translation, ADR 0005 item
// 8b) renders its text in the Message cell — the locale tag never leaks into
// the journal.
TEST_F(EventFloodGroupingTest, ALocalizedMessageRendersItsText) {
  historical_event_model_.AddEvent(
      {.event_id = 1,
       .time = scada::DateTime{},
       .source_node_id = node_id_,
       .message = scada::LocalizedText{"ru", u"Значение в норме"}});
  Rebuild();

  ASSERT_EQ(model_.GetRowCount(), 1);
  scada::aui::TableCell cell{.row = 0, .column_id = EventColumnMessage};
  model_.GetCell(cell);
  EXPECT_EQ(cell.text, u"Значение в норме");
}

// Below the flood threshold nothing changes: a quiet journal is a plain list,
// one row per event, so grouping never surprises an operator who is not buried.
TEST_F(EventFloodGroupingTest, ABacklogBelowTheThresholdIsNotGrouped) {
  AddRepeats(u"comms lost", events::kAlarmFloodThreshold, /*first_id=*/1);
  Rebuild();

  EXPECT_FALSE(model_.grouped());
  EXPECT_EQ(model_.GetRowCount(), events::kAlarmFloodThreshold);
}

TEST_F(EventFloodGroupingTest, AFloodCollapsesRepeatsIntoOneCountedRow) {
  const int count = events::kAlarmFloodThreshold + 5;
  AddRepeats(u"comms lost", count, /*first_id=*/1);
  Rebuild();

  EXPECT_TRUE(model_.grouped());
  // The flood reads as one line carrying its count, not as `count` lines.
  ASSERT_EQ(model_.GetRowCount(), 1);
  EXPECT_EQ(model_.group_count_at(0), count);

  scada::aui::TableCell cell{.row = 0, .column_id = EventColumnMessage};
  model_.GetCell(cell);
  EXPECT_EQ(cell.text, events::FormatGroupedMessage(u"comms lost", count));
}

// Grouping must not swallow the alarm that only happened once — that is the one
// the flood would otherwise bury.
TEST_F(EventFloodGroupingTest, AOneOffAlarmStaysItsOwnRowDuringAFlood) {
  AddRepeats(u"comms lost", events::kAlarmFloodThreshold + 5, /*first_id=*/1);
  AddRepeats(u"transformer overheating", 1, /*first_id=*/100);
  Rebuild();

  ASSERT_EQ(model_.GetRowCount(), 2);

  int singles = 0;
  for (int row = 0; row < model_.GetRowCount(); ++row) {
    if (model_.group_count_at(row) == 1) {
      ++singles;
      EXPECT_EQ(model_.event_at(row).message, u"transformer overheating");
    }
  }
  EXPECT_EQ(singles, 1);
}

// The row shows the latest occurrence: during a flood the operator is looking
// at what is happening now, not at when the chattering started.
TEST_F(EventFloodGroupingTest, AGroupedRowShowsItsNewestOccurrence) {
  const int count = events::kAlarmFloodThreshold + 3;
  AddRepeats(u"comms lost", count, /*first_id=*/1);
  Rebuild();

  ASSERT_EQ(model_.GetRowCount(), 1);
  EXPECT_EQ(model_.event_at(0).time,
            scada::DateTime{} +
                std::chrono::seconds(count - 1));
}

// The live rows collapse too. Local events are the handle: they are
// unacknowledged on arrival, share a source and message, and — unlike the
// historical rows — can actually be acknowledged, so the whole lifecycle of a
// collapsed live row is observable here.
TEST_F(EventFloodGroupingTest, LiveRepeatsCollapseDuringAFlood) {
  const int count = events::kAlarmFloodThreshold + 4;
  for (int i = 0; i < count; ++i)
    local_events_.ReportEvent(LocalEvents::SEV_ERROR, u"comms lost");
  Rebuild();

  ASSERT_TRUE(model_.grouped());
  EXPECT_EQ(model_.GetRowCount(), 1);
  EXPECT_EQ(model_.group_count_at(0), count);
}

// An arrival during a flood bumps the count of the alarm it repeats instead of
// growing the list — the whole point of grouping.
TEST_F(EventFloodGroupingTest, AnArrivalFoldsIntoTheGroupItRepeats) {
  const int count = events::kAlarmFloodThreshold + 4;
  for (int i = 0; i < count; ++i)
    local_events_.ReportEvent(LocalEvents::SEV_ERROR, u"comms lost");
  Rebuild();
  ASSERT_EQ(model_.GetRowCount(), 1);

  local_events_.ReportEvent(LocalEvents::SEV_ERROR, u"comms lost");

  EXPECT_EQ(model_.GetRowCount(), 1);
  EXPECT_EQ(model_.group_count_at(0), count + 1);
  // The arrival is the newest occurrence, so it is the one on display.
  EXPECT_EQ(model_.event_at(0).event_id,
            local_events_.events().back()->event_id);
}

// A different alarm arriving mid-flood must not be folded away — it is exactly
// the one the flood would otherwise bury.
TEST_F(EventFloodGroupingTest, ADifferentAlarmArrivingMidFloodGetsItsOwnRow) {
  const int count = events::kAlarmFloodThreshold + 4;
  for (int i = 0; i < count; ++i)
    local_events_.ReportEvent(LocalEvents::SEV_ERROR, u"comms lost");
  Rebuild();
  ASSERT_EQ(model_.GetRowCount(), 1);

  local_events_.ReportEvent(LocalEvents::SEV_ERROR, u"transformer overheating");

  ASSERT_EQ(model_.GetRowCount(), 2);
  EXPECT_EQ(model_.group_count_at(1), 1);
}

// Acknowledging one occupant of a collapsed row leaves the rest of the group in
// place. Removing the whole row would drop alarms nobody has acknowledged.
TEST_F(EventFloodGroupingTest, AcknowledgingOneOccurrenceKeepsTheRest) {
  const int count = events::kAlarmFloodThreshold + 4;
  for (int i = 0; i < count; ++i)
    local_events_.ReportEvent(LocalEvents::SEV_ERROR, u"comms lost");
  Rebuild();
  ASSERT_EQ(model_.group_count_at(0), count);

  local_events_.AcknowledgeEvent(local_events_.events().front()->event_id);

  ASSERT_EQ(model_.GetRowCount(), 1);
  EXPECT_EQ(model_.group_count_at(0), count - 1);
}

// Acknowledging the occurrence that is on display promotes another one, rather
// than leaving the row pointing at an event the storage has already destroyed.
TEST_F(EventFloodGroupingTest, AcknowledgingTheDisplayedOccurrencePromotes) {
  const int count = events::kAlarmFloodThreshold + 4;
  for (int i = 0; i < count; ++i)
    local_events_.ReportEvent(LocalEvents::SEV_ERROR, u"comms lost");
  Rebuild();
  const scada::EventId displayed = model_.event_at(0).event_id;

  local_events_.AcknowledgeEvent(displayed);

  ASSERT_EQ(model_.GetRowCount(), 1);
  EXPECT_NE(model_.event_at(0).event_id, displayed);
  EXPECT_EQ(model_.group_count_at(0), count - 1);
}

// Acknowledging a collapsed row acknowledges every occurrence it stands for —
// otherwise the operator clears what they can see and the rest of the count
// stays unacknowledged behind it.
TEST_F(EventFloodGroupingTest, AcknowledgingAGroupAcknowledgesEveryOccurrence) {
  const int count = events::kAlarmFloodThreshold + 4;
  for (int i = 0; i < count; ++i)
    local_events_.ReportEvent(LocalEvents::SEV_ERROR, u"comms lost");
  Rebuild();
  ASSERT_EQ(model_.GetRowCount(), 1);
  ASSERT_EQ(local_events_.events().size(), static_cast<size_t>(count));

  model_.AcknowledgeRow(0);

  EXPECT_TRUE(local_events_.events().empty());
  EXPECT_EQ(model_.GetRowCount(), 0);
}

// Grouping is a property of the situation, not a mode: once the backlog is
// worked back below the threshold the rows expand again on their own.
TEST_F(EventFloodGroupingTest, RowsExpandAgainOnceTheFloodIsWorkedOff) {
  const int count = events::kAlarmFloodThreshold + 4;
  for (int i = 0; i < count; ++i)
    local_events_.ReportEvent(LocalEvents::SEV_ERROR, u"comms lost");
  Rebuild();
  ASSERT_TRUE(model_.grouped());

  // Acknowledge just enough to end the flood.
  for (int i = 0; i < 4; ++i)
    local_events_.AcknowledgeEvent(local_events_.events().front()->event_id);

  EXPECT_FALSE(model_.grouped());
  EXPECT_EQ(model_.GetRowCount(), events::kAlarmFloodThreshold);
  for (int row = 0; row < model_.GetRowCount(); ++row)
    EXPECT_EQ(model_.group_count_at(row), 1);
}

// Acknowledging a multi-row selection must acknowledge exactly the alarms that
// were selected. Acknowledging the first row here drops the backlog out of
// flood, which expands the rows and renumbers them — so a loop that re-read row
// indices as it went would acknowledge the wrong alarm, or run off the end.
TEST_F(EventFloodGroupingTest, AcknowledgingAMultiRowSelectionIsIndexSafe) {
  const int count = events::kAlarmFloodThreshold + 4;
  for (int i = 0; i < count; ++i)
    local_events_.ReportEvent(LocalEvents::SEV_ERROR, u"comms lost");
  local_events_.ReportEvent(LocalEvents::SEV_ERROR, u"transformer overheating");
  Rebuild();
  ASSERT_TRUE(model_.grouped());
  ASSERT_EQ(model_.GetRowCount(), 2);

  const int selection[] = {0, 1};
  model_.AcknowledgeRows(selection);

  EXPECT_TRUE(local_events_.events().empty());
  EXPECT_EQ(model_.GetRowCount(), 0);
}

// A record of the journal must contain what happened, not how the display
// folded it: exports and printouts read the occurrences, so a collapsed row
// contributes one entry per occurrence and none of them carries a "×N".
TEST_F(EventFloodGroupingTest, ExpandedRowsCarryEveryOccurrence) {
  const int count = events::kAlarmFloodThreshold + 4;
  AddRepeats(u"comms lost", count, /*first_id=*/1);
  AddRepeats(u"transformer overheating", 1, /*first_id=*/100);
  Rebuild();

  // Displayed: one collapsed row plus the one-off.
  ASSERT_TRUE(model_.grouped());
  ASSERT_EQ(model_.GetRowCount(), 2);

  ExpandedEventModel expanded{model_};
  EXPECT_EQ(expanded.GetRowCount(), count + 1);

  int comms_lost = 0;
  for (int row = 0; row < expanded.GetRowCount(); ++row) {
    const std::u16string message =
        expanded.GetCellText(row, EventColumnMessage);
    // No occurrence is rendered with a count — each one stands alone.
    EXPECT_EQ(message.find(u"\u00d7"), std::u16string::npos) << row;
    if (message == u"comms lost")
      ++comms_lost;
  }
  EXPECT_EQ(comms_lost, count);
}

// Every occurrence keeps its own timestamp; the export must not repeat the
// representative's row `count` times.
TEST_F(EventFloodGroupingTest, ExpandedRowsKeepTheirOwnTimes) {
  const int count = events::kAlarmFloodThreshold + 4;
  AddRepeats(u"comms lost", count, /*first_id=*/1);
  Rebuild();
  ASSERT_EQ(model_.GetRowCount(), 1);

  ExpandedEventModel expanded{model_};
  ASSERT_EQ(expanded.GetRowCount(), count);

  std::set<std::u16string> times;
  for (int row = 0; row < expanded.GetRowCount(); ++row)
    times.insert(expanded.GetCellText(row, EventColumnTime));
  EXPECT_EQ(times.size(), static_cast<size_t>(count));
}

// With nothing collapsed, expanding is a no-op — the export of a quiet journal
// is unchanged.
TEST_F(EventFloodGroupingTest, ExpandingAnUngroupedJournalChangesNothing) {
  AddRepeats(u"comms lost", 3, /*first_id=*/1);
  Rebuild();
  ASSERT_FALSE(model_.grouped());

  ExpandedEventModel expanded{model_};
  ASSERT_EQ(expanded.GetRowCount(), model_.GetRowCount());
  for (int row = 0; row < model_.GetRowCount(); ++row) {
    EXPECT_EQ(expanded.GetCellText(row, EventColumnMessage),
              model_.GetCellText(row, EventColumnMessage));
    EXPECT_EQ(expanded.GetCellText(row, EventColumnTime),
              model_.GetCellText(row, EventColumnTime));
  }
}

// The alarm chrome (journal footer + pending-dot column) reuses the flood
// fixture's historical journal: an empty current surface plus directly-seeded
// history.
using EventAlarmChromeTest = EventFloodGroupingTest;

// The footer summary counts the displayed unacknowledged occurrences and
// tracks the highest severity among them; acknowledged history stays out.
TEST_F(EventAlarmChromeTest, AlarmSummaryCountsPendingAndHighestSeverity) {
  historical_event_model_.AddEvent({.event_id = 1,
                                    .severity = scada::kSeverityWarning,
                                    .source_node_id = node_id_,
                                    .message = u"warn"});
  historical_event_model_.AddEvent({.event_id = 2,
                                    .severity = scada::kSeverityCritical,
                                    .source_node_id = node_id_,
                                    .message = u"crit"});
  historical_event_model_.AddEvent({.event_id = 3,
                                    .severity = 1000,
                                    .source_node_id = node_id_,
                                    .message = u"done",
                                    .acked = true});
  Rebuild();

  EXPECT_EQ(
      model_.GetAlarmSummary(),
      (EventTableModel::AlarmSummary{
          .unacknowledged = 2, .max_severity = scada::kSeverityCritical}));
}

// An empty journal summarizes as the calm zero state.
TEST_F(EventAlarmChromeTest, EmptyJournalSummarizesCalm) {
  Rebuild();
  EXPECT_EQ(model_.GetAlarmSummary(), EventTableModel::AlarmSummary{});
}

// The leading pending-dot cell marks unacknowledged rows; acknowledged rows
// stay empty, so the actionable rows read at a glance.
TEST_F(EventAlarmChromeTest, PendingDotMarksUnacknowledgedRows) {
  historical_event_model_.AddEvent({.event_id = 1,
                                    .severity = scada::kSeverityCritical,
                                    .source_node_id = node_id_,
                                    .message = u"crit"});
  historical_event_model_.AddEvent({.event_id = 2,
                                    .source_node_id = node_id_,
                                    .message = u"done",
                                    .acked = true});
  Rebuild();
  ASSERT_EQ(model_.GetRowCount(), 2);

  for (int row = 0; row < model_.GetRowCount(); ++row) {
    const bool pending = !model_.event_at(row).acked;
    EXPECT_EQ(model_.GetCellText(row, EventColumnUnacked),
              pending ? std::u16string{u"\u25CF"} : std::u16string{});
  }
}

// A live unacknowledged alarm that also appears in the read history is one
// event, not two rows: the journal drops the historical copy by event id
// (regression: the alarm chrome surfaced live+history duplicates of every
// pending alarm).
TEST_F(EventAlarmChromeTest, LiveAlarmIsNotDuplicatedByItsHistoryCopy) {
  const scada::EventId event_id = 7;
  const scada::Event live{.event_id = event_id,
                          .severity = scada::kSeverityCritical,
                          .source_node_id = node_id_,
                          .message = u"crit"};
  empty_current_.try_emplace(event_id, live);
  historical_event_model_.AddEvent(live);
  historical_event_model_.AddEvent({.event_id = 8,
                                    .source_node_id = node_id_,
                                    .message = u"other",
                                    .acked = true});
  Rebuild();

  ASSERT_EQ(model_.GetRowCount(), 2);
  EXPECT_EQ(
      model_.GetAlarmSummary(),
      (EventTableModel::AlarmSummary{
          .unacknowledged = 1, .max_severity = scada::kSeverityCritical}));
}

// The Areas sidebar's counts: unacknowledged occurrences attribute to the
// area containing their source, the total spans every area (and sources
// outside all of them), acknowledged events count nowhere — and the active
// area filter does not skew the other areas' counts.
TEST_F(EventAlarmChromeTest, CountsUnacknowledgedByArea) {
  const scada::NodeId area_a{100, scada::NamespaceIndexes::TIT};
  const scada::NodeId area_b{101, scada::NamespaceIndexes::TIT};
  const scada::NodeId in_a{102, scada::NamespaceIndexes::TIT};
  node_service_.Add({.node_id = area_a,
                     .type_definition_id = scada::id::FolderType,
                     .attributes = {.browse_name = "a", .display_name = u"A"}});
  node_service_.Add({.node_id = area_b,
                     .type_definition_id = scada::id::FolderType,
                     .attributes = {.browse_name = "b", .display_name = u"B"}});
  node_service_.Add(
      {.node_id = in_a,
       .type_definition_id = scada::id::FolderType,
       .parent_id = area_a,
       .reference_type_id = scada::id::Organizes,
       .attributes = {.browse_name = "n2", .display_name = u"N2"}});

  // Two pending alarms under area A (one direct, one via containment), one
  // acknowledged under A, and one pending outside every area.
  historical_event_model_.AddEvent(
      {.event_id = 1, .source_node_id = in_a, .message = u"m1"});
  historical_event_model_.AddEvent(
      {.event_id = 2, .source_node_id = area_a, .message = u"m2"});
  historical_event_model_.AddEvent(
      {.event_id = 3, .source_node_id = in_a, .message = u"m3", .acked = true});
  historical_event_model_.AddEvent(
      {.event_id = 4, .source_node_id = node_id_, .message = u"m4"});
  Rebuild();

  const scada::NodeId areas[] = {area_a, area_b};
  EXPECT_EQ(model_.CountUnacknowledgedByArea(areas),
            (EventTableModel::AreaCounts{.total = 3, .per_area = {2, 0}}));

  // Filtering to area B leaves A's count intact.
  model_.AddFilteredItem(area_b);
  EXPECT_EQ(model_.CountUnacknowledgedByArea(areas),
            (EventTableModel::AreaCounts{.total = 3, .per_area = {2, 0}}));
}
