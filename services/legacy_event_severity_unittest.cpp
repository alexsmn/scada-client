#include "services/legacy_event_severity.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "model/data_items_node_ids.h"
#include "scada/attribute_service.h"
#include "scada/history_service.h"
#include "scada/monitored_item.h"
#include "scada/monitored_item_service.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <any>
#include <variant>

using namespace testing;

namespace {

// Answers the version probe the way a Server does: a 2.5.7 Server has never
// heard of the per-item Control object's type and refuses it as an unknown
// node, a 2.6 Server returns its node class. `whole_call_error` makes the
// whole Read fail instead, as a dropped session does.
class FakeAttributeService final : public scada::AttributeService {
 public:
  explicit FakeAttributeService(bool before_2_6) : before_2_6_{before_2_6} {}

  scada::CoStatusOr<std::vector<scada::DataValue>> Read(
      scada::ServiceContext,
      std::vector<scada::ReadValueId> inputs) override {
    ++read_count;
    if (whole_call_error)
      co_return scada::Status{*whole_call_error};
    std::vector<scada::DataValue> results;
    for (const scada::ReadValueId& input : inputs) {
      const bool known =
          !before_2_6_ ||
          input.node_id != scada::data_items::id::DataItemControlType;
      results.push_back(
          known ? scada::MakeReadResult(scada::NodeClass::ObjectType)
                : scada::MakeReadError(scada::StatusCode::Bad_WrongNodeId));
    }
    co_return results;
  }

  scada::CoStatusOr<std::vector<scada::StatusCode>> Write(
      scada::ServiceContext,
      std::vector<scada::WriteValue> inputs) override {
    co_return std::vector<scada::StatusCode>(
        inputs.size(), scada::StatusCode::Bad_NotSupported);
  }

  int read_count = 0;
  std::optional<scada::StatusCode> whole_call_error;

 private:
  const bool before_2_6_;
};

// Returns `events` from every event history read, as the Server stored them.
class FakeHistoryService final : public scada::HistoryService {
 public:
  scada::CoStatusOr<scada::HistoryReadRawResult> HistoryReadRaw(
      scada::HistoryReadRawDetails) override {
    co_return scada::HistoryReadRawResult{};
  }

  scada::CoStatusOr<scada::HistoryReadEventsResult> HistoryReadEvents(
      scada::NodeId,
      scada::Time,
      scada::Time,
      scada::EventFilter) override {
    co_return scada::HistoryReadEventsResult{.events = events};
  }

  std::vector<scada::Event> events;
};

// Delivers `batch` on the first ReadNext and nothing after it.
class FakeSubscription final : public scada::MonitoredItemSubscription {
 public:
  explicit FakeSubscription(std::vector<scada::MonitoredItemNotification> batch)
      : batch_{std::move(batch)} {}

  Awaitable<std::vector<scada::MonitoredItemCreateResult>> AddItems(
      std::vector<scada::MonitoredItemCreateRequest>) override {
    co_return std::vector<scada::MonitoredItemCreateResult>{};
  }

  Awaitable<std::vector<scada::Status>> RemoveItems(
      std::span<const scada::MonitoredItemId>) override {
    co_return std::vector<scada::Status>{};
  }

  scada::CoStatusOr<std::vector<scada::MonitoredItemNotification>> ReadNext(
      std::size_t) override {
    co_return std::exchange(batch_, {});
  }

  void Close(scada::Status) override {}

 private:
  std::vector<scada::MonitoredItemNotification> batch_;
};

class FakeMonitoredItemService final : public scada::MonitoredItemService {
 public:
  scada::StatusOr<std::unique_ptr<scada::MonitoredItemSubscription>>
  CreateSubscription(scada::ServiceContext,
                     scada::MonitoredItemSubscriptionOptions) override {
    return std::unique_ptr<scada::MonitoredItemSubscription>{
        std::make_unique<FakeSubscription>(std::move(batch))};
  }

  std::vector<scada::MonitoredItemNotification> batch;
};

scada::Event MakeEvent(unsigned severity) {
  return scada::Event{.severity = severity};
}

// A Server of either version behind the decorator, the way the Client sees it
// after login.
class LegacyEventSeverityTest : public Test {
 protected:
  void ConnectTo(bool before_2_6) {
    attribute_service_ = std::make_shared<FakeAttributeService>(before_2_6);
    services_ = RescaleLegacyEventSeverities(
        DataServices{.history_service_ = history_service_,
                     .attribute_service_ = attribute_service_,
                     .monitored_item_service_ = monitored_item_service_});
  }

  std::vector<unsigned> ReadHistorySeverities() {
    auto result = WaitAwaitable(
        executor_, services_.history_service_->HistoryReadEvents(
                       scada::id::Server, {}, {}, scada::EventFilter{}));
    EXPECT_TRUE(result.ok());
    std::vector<unsigned> severities;
    for (const scada::Event& event : result->events)
      severities.push_back(event.severity);
    return severities;
  }

  std::vector<scada::MonitoredItemNotification> ReadLiveBatch() {
    auto subscription =
        services_.monitored_item_service_->CreateSubscription({}, {});
    EXPECT_TRUE(subscription.ok());
    auto result = WaitAwaitable(executor_, (*subscription)->ReadNext(1024));
    EXPECT_TRUE(result.ok());
    return std::move(*result);
  }

  TestExecutor executor_;
  std::shared_ptr<FakeAttributeService> attribute_service_;
  std::shared_ptr<FakeHistoryService> history_service_ =
      std::make_shared<FakeHistoryService>();
  std::shared_ptr<FakeMonitoredItemService> monitored_item_service_ =
      std::make_shared<FakeMonitoredItemService>();
  DataServices services_;
};

TEST(RescaleLegacySeverityTest, MapsTheOldScaleOntoTheNewBands) {
  // 2.5.7's Warning and Critical land exactly on 2.6's.
  EXPECT_EQ(RescaleLegacySeverity(60), scada::kSeverityWarning);
  EXPECT_EQ(RescaleLegacySeverity(80), scada::kSeverityCritical);
  EXPECT_EQ(RescaleLegacySeverity(50), scada::kSeverityNormal);
  EXPECT_EQ(RescaleLegacySeverity(100), scada::kSeverityMax);
  // 0 ("silent" in 2.5.7) is not a valid OPC UA severity.
  EXPECT_EQ(RescaleLegacySeverity(0), scada::kSeverityMin);
  // Out of range stays in range, without wrapping.
  EXPECT_EQ(RescaleLegacySeverity(101), scada::kSeverityMax);
  EXPECT_EQ(RescaleLegacySeverity(0xFFFFFFFFu), scada::kSeverityMax);
}

TEST_F(LegacyEventSeverityTest, RescalesHistoryFromAServerBefore26) {
  history_service_->events = {MakeEvent(80), MakeEvent(60), MakeEvent(0)};
  ConnectTo(/*before_2_6=*/true);

  EXPECT_THAT(ReadHistorySeverities(),
              ElementsAre(scada::kSeverityCritical, scada::kSeverityWarning,
                          scada::kSeverityMin));
}

TEST_F(LegacyEventSeverityTest, LeavesHistoryFromA26ServerAlone) {
  // 80 from a 2.6 Server is a genuine low severity, not 2.5.7's Critical.
  history_service_->events = {MakeEvent(800), MakeEvent(80)};
  ConnectTo(/*before_2_6=*/false);

  EXPECT_THAT(ReadHistorySeverities(), ElementsAre(800u, 80u));
}

TEST_F(LegacyEventSeverityTest, RescalesLiveEventsFromAServerBefore26) {
  scada::DeviceFrameEvent frame;
  frame.base.severity = 60;
  monitored_item_service_->batch = {
      scada::EventNotification{.event = std::any{MakeEvent(80)}},
      scada::EventNotification{.event = std::any{frame}},
      scada::DataChangeNotification{
          .value =
              scada::DataValue{scada::Variant{scada::Int32{80}}, {}, {}, {}}},
  };
  ConnectTo(/*before_2_6=*/true);

  auto batch = ReadLiveBatch();

  ASSERT_EQ(batch.size(), 3u);
  EXPECT_EQ(std::any_cast<scada::Event>(
                std::get<scada::EventNotification>(batch[0]).event)
                .severity,
            scada::kSeverityCritical);
  EXPECT_EQ(std::any_cast<scada::DeviceFrameEvent>(
                std::get<scada::EventNotification>(batch[1]).event)
                .base.severity,
            scada::kSeverityWarning);
  // A value is not a severity, whatever its number.
  EXPECT_EQ(std::get<scada::DataChangeNotification>(batch[2]).value.value,
            scada::Variant{scada::Int32{80}});
}

TEST_F(LegacyEventSeverityTest, LeavesLiveEventsFromA26ServerAlone) {
  monitored_item_service_->batch = {
      scada::EventNotification{.event = std::any{MakeEvent(80)}}};
  ConnectTo(/*before_2_6=*/false);

  auto batch = ReadLiveBatch();

  ASSERT_EQ(batch.size(), 1u);
  EXPECT_EQ(std::any_cast<scada::Event>(
                std::get<scada::EventNotification>(batch[0]).event)
                .severity,
            80u);
}

TEST_F(LegacyEventSeverityTest, AsksTheServerOnceAndOnlyForEvents) {
  monitored_item_service_->batch = {scada::DataChangeNotification{}};
  history_service_->events = {MakeEvent(80)};
  ConnectTo(/*before_2_6=*/true);

  // A batch with no events needs no answer.
  ReadLiveBatch();
  EXPECT_EQ(attribute_service_->read_count, 0);

  ReadHistorySeverities();
  ReadHistorySeverities();
  EXPECT_EQ(attribute_service_->read_count, 1);
}

TEST_F(LegacyEventSeverityTest, AFailedProbeDecidesNothingAndIsAskedAgain) {
  history_service_->events = {MakeEvent(80)};
  ConnectTo(/*before_2_6=*/true);
  attribute_service_->whole_call_error = scada::StatusCode::Bad_Disconnected;

  // Undecided: delivered as sent rather than guessed at.
  EXPECT_THAT(ReadHistorySeverities(), ElementsAre(80u));

  attribute_service_->whole_call_error.reset();
  EXPECT_THAT(ReadHistorySeverities(), ElementsAre(scada::kSeverityCritical));
  EXPECT_EQ(attribute_service_->read_count, 2);
}

TEST_F(LegacyEventSeverityTest, NoAttributeServiceMeansNothingToRescale) {
  history_service_->events = {MakeEvent(80)};
  services_ = RescaleLegacyEventSeverities(
      DataServices{.history_service_ = history_service_});

  EXPECT_THAT(ReadHistorySeverities(), ElementsAre(80u));
}

}  // namespace
