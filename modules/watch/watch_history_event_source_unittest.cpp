#include "modules/watch/watch_history_event_source.h"

#include "base/async_completion.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "node_service/static/static_node_service.h"
#include "scada/history_service_mock.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

constexpr scada::NodeId kDeviceTypeId{8001, 1};
constexpr scada::NodeId kDeviceId{8002, 1};

scada::Event MakeEvent(scada::EventId event_id) {
  return scada::Event{
      .event_id = event_id,
      .time = scada::DateTime::UnixEpoch() +
              scada::Duration::FromSeconds(static_cast<int64_t>(event_id)),
      .message = u"event"};
}

class RecordingDelegate : public WatchEventSource::Delegate {
 public:
  void OnEvent(const scada::Event& event) override { events.emplace_back(event); }
  void OnError(const scada::Status& status) override { errors.emplace_back(status); }

  std::vector<scada::Event> events;
  std::vector<scada::Status> errors;
};

class WatchHistoryEventSourceTest : public Test {
 protected:
  WatchHistoryEventSourceTest()
      : node_service_{scada::services{.history_service = &history_service_}},
        source_{WatchHistorySourceContext{executor_, node_service_}} {
    node_service_.Add(
        {.node_id = kDeviceTypeId, .node_class = scada::NodeClass::ObjectType});
    node_service_.Add({.node_id = kDeviceId,
                       .node_class = scada::NodeClass::Object,
                       .type_definition_id = kDeviceTypeId});
  }

  TestExecutor executor_;
  StrictMock<scada::MockHistoryService> history_service_;
  StaticNodeService node_service_;
  WatchHistoryEventSource source_;
  RecordingDelegate delegate_;
};

}  // namespace

TEST_F(WatchHistoryEventSourceTest, StartDeliversHistoryEvents) {
  const auto from = scada::DateTime::UnixEpoch() + scada::Duration::FromSeconds(1);
  const auto to = scada::DateTime::UnixEpoch() + scada::Duration::FromSeconds(2);

  EXPECT_CALL(history_service_, HistoryReadEvents(kDeviceId, from, to, _))
      .WillOnce([](scada::NodeId, scada::DateTime, scada::DateTime,
                   scada::EventFilter)
                    -> Awaitable<scada::HistoryReadEventsResult> {
        co_return scada::HistoryReadEventsResult{
            .status = scada::StatusCode::Good,
            .events = {MakeEvent(1), MakeEvent(2)}};
      });

  source_.Start(kDeviceId, {from, to}, delegate_);
  Drain(executor_);

  EXPECT_THAT(delegate_.events,
              ElementsAre(Field(&scada::Event::event_id, 1),
                          Field(&scada::Event::event_id, 2)));
  EXPECT_TRUE(delegate_.errors.empty());
}

TEST_F(WatchHistoryEventSourceTest, NewStartCancelsStaleHistoryDelivery) {
  base::AsyncCompletion completion{executor_};
  scada::HistoryReadEventsResult result;
  bool history_read_started = false;

  EXPECT_CALL(history_service_, HistoryReadEvents(kDeviceId, _, _, _))
      .WillOnce([&](scada::NodeId, scada::DateTime, scada::DateTime,
                    scada::EventFilter)
                    -> Awaitable<scada::HistoryReadEventsResult> {
        history_read_started = true;
        co_await completion.Wait();
        co_return result;
      });

  source_.Start(kDeviceId,
                {scada::DateTime::UnixEpoch(),
                 scada::DateTime::UnixEpoch() + scada::Duration::FromSeconds(1)},
                delegate_);
  Drain(executor_);

  ASSERT_TRUE(history_read_started);
  source_.Start(scada::NodeId{}, {scada::DateTime::UnixEpoch(),
                                  scada::DateTime::UnixEpoch()},
                delegate_);
  result = scada::HistoryReadEventsResult{.status = scada::StatusCode::Good,
                                          .events = {MakeEvent(1)}};
  completion.Complete();
  Drain(executor_);

  EXPECT_TRUE(delegate_.events.empty());
  EXPECT_TRUE(delegate_.errors.empty());
}

TEST_F(WatchHistoryEventSourceTest, NullDeviceDoesNotReadHistory) {
  EXPECT_CALL(history_service_, HistoryReadEvents(_, _, _, _)).Times(0);

  source_.Start(scada::NodeId{}, {scada::DateTime::UnixEpoch(),
                                  scada::DateTime::UnixEpoch()},
                delegate_);
  Drain(executor_);

  EXPECT_TRUE(delegate_.events.empty());
}
