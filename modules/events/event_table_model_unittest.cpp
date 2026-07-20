#include "events/event_table_model.h"

#include "base/test/test_executor.h"
#include "events/current_event_model.h"
#include "events/event_severity.h"
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

#include "base/utf_convert.h"
#include <gmock/gmock.h>

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
        .node_id = nodes.node_id(index % nodes.count),
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

  historical_event_model.AddEvent({.event_id = 1, .node_id = node_id});
  historical_event_model.AddEvent({.event_id = 2, .node_id = node_id});
  historical_event_model.AddEvent(
      {.event_id = 3, .node_id = node_id, .acked = true});

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

  const auto ack_time = scada::DateTime::Now();
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
};

TEST_F(EventJournalAlarmSurfaceTest, LegacySeverityCellIsTheBareNumber) {
  FirstEvent().severity = scada::kSeverityCritical;
  Init();

  EXPECT_EQ(CellText(EventColumnSeverity), u"80");
}

TEST_F(EventJournalAlarmSurfaceTest, ThemedSeverityCellNamesTheAlarmBand) {
  FirstEvent().severity = scada::kSeverityCritical;
  Init();
  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kDark);

  // The band is named as well as numbered, so severity does not depend on the
  // row's colour alone.
  EXPECT_EQ(CellText(EventColumnSeverity),
            events::SeverityLevelLabel(scada::aui::SeverityLevel::kCritical) +
                u" 80");
}

// A routine event has no alarm band, so there is nothing to name — it stays the
// bare number rather than gaining a misleading label.
TEST_F(EventJournalAlarmSurfaceTest, ThemedRoutineSeverityCellStaysTheNumber) {
  FirstEvent().severity = scada::kSeverityNormal;
  Init();
  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kDark);

  EXPECT_EQ(CellText(EventColumnSeverity), u"50");
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
  event.acknowledged_time = scada::DateTime::Now();
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
