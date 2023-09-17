#include "components/events/event_table_model.h"

#include "base/test/test_executor.h"
#include "common/node_event_provider_mock.h"
#include "components/events/current_event_model.h"
#include "components/events/historical_event_model.h"
#include "components/events/local_event_model.h"
#include "model/namespaces.h"
#include "model/node_id_util.h"
#include "node_service/node_service_mock.h"
#include "node_service/static/static_node_service.h"
#include "scada/history_service_mock.h"
#include "services/local_events.h"

#include <boost/locale/encoding_utf.hpp>
#include <gmock/gmock.h>

using namespace testing;

namespace {

struct TestNodeGenerator {
  scada::NodeId node_id(int index) const {
    return {static_cast<scada::NumericId>(index), NamespaceIndexes::TIT};
  }

  scada::LocalizedText display_name(int index) const {
    return boost::locale::conv::utf_to_utf<char16_t>(
        std::format("Event {}", index + 1));
  }

  const int count = 3;
};

NodeEventProvider::EventContainer GenerateEvents(const TestNodeGenerator& nodes,
                                                 int start,
                                                 int count) {
  NodeEventProvider::EventContainer events;
  for (int i = 0; i < count; ++i) {
    int index = i + start;
    scada::Event event{
        .node_id = nodes.node_id(index % nodes.count),
        .message = boost::locale::conv::utf_to_utf<char16_t>(
            std::format("Event {}", index + 1)),
        .acknowledge_id = static_cast<scada::EventAcknowledgeId>(index + 1)};
    events.try_emplace(event.acknowledge_id, std::move(event));
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

  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();

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

TEST_F(EventTableModelTest, CurrentEvents_AckEvents) {
  Init();

  const auto ack_time = scada::DateTime::Now();
  const int ack_count = 10;

  std::vector<const scada::Event*> event_ptrs;
  for (auto& event :
       test_events_ | std::views::values | std::views::take(ack_count)) {
    event.acked = true;
    event.acknowledged_time = ack_time;
    event.acknowledged_user_id = {1, NamespaceIndexes::USER};
    event_ptrs.emplace_back(&event);
  }

  event_observer_->OnEvents(event_ptrs);

  ValidateEvents();
}
