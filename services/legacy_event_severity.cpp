#include "services/legacy_event_severity.h"

#include "model/data_items_node_ids.h"
#include "scada/attribute_service.h"
#include "scada/history_service.h"
#include "scada/monitored_item.h"
#include "scada/monitored_item_service.h"
#include "services/server_compatibility.h"

#include <algorithm>
#include <utility>
#include <variant>

scada::EventSeverity RescaleLegacySeverity(unsigned severity) {
  // Widened before multiplying so a nonsense value cannot wrap back into range.
  const unsigned long long scaled =
      static_cast<unsigned long long>(severity) * 10;
  return static_cast<scada::EventSeverity>(std::clamp<unsigned long long>(
      scaled, scada::kSeverityMin, scada::kSeverityMax));
}

void RescaleLegacySeverities(std::span<scada::Event> events) {
  for (scada::Event& event : events)
    event.severity = RescaleLegacySeverity(event.severity);
}

LegacySeverityProbe::LegacySeverityProbe(
    std::shared_ptr<scada::AttributeService> attribute_service)
    : attribute_service_{std::move(attribute_service)} {}

Awaitable<bool> LegacySeverityProbe::IsServerBefore26() {
  if (before_2_6_)
    co_return *before_2_6_;
  if (!attribute_service_)
    co_return false;

  // The 2.5.7 Server answers a Read of a node no service owns with a per-value
  // Bad_WrongNodeId (`BaseNodeManager::Read` -> `MakeReadError`, read at
  // b122fd7ad, the 2.5.7 source). The whole-call status is checked too, for a
  // transport that reports it there.
  auto result = co_await attribute_service_->Read(
      {},
      {scada::ReadValueId{.node_id = scada::data_items::id::DataItemControlType,
                          .attribute_id = scada::AttributeId::NodeClass}});
  if (!result.ok()) {
    if (IsMissingOnServer(result.status()))
      before_2_6_ = true;
    co_return before_2_6_.value_or(false);
  }
  if (result->empty())
    co_return false;
  before_2_6_ = IsMissingOnServer(scada::Status{result->front().status_code});
  co_return *before_2_6_;
}

namespace {

// Rescales the events a history read returns.
class LegacySeverityHistoryService final : public scada::HistoryService {
 public:
  LegacySeverityHistoryService(std::shared_ptr<scada::HistoryService> inner,
                               std::shared_ptr<LegacySeverityProbe> probe)
      : inner_{std::move(inner)}, probe_{std::move(probe)} {}

  scada::CoStatusOr<scada::HistoryReadRawResult> HistoryReadRaw(
      scada::HistoryReadRawDetails details) override {
    return inner_->HistoryReadRaw(std::move(details));
  }

  scada::CoStatusOr<scada::HistoryReadEventsResult> HistoryReadEvents(
      scada::NodeId node_id,
      scada::Time from,
      scada::Time to,
      scada::EventFilter filter) override {
    // Both held by value in the frame, so a decorator replaced mid-read
    // cannot leave the coroutine reading a dead member.
    auto inner = inner_;
    auto probe = probe_;
    auto result = co_await inner->HistoryReadEvents(std::move(node_id), from,
                                                    to, std::move(filter));
    if (result.ok() && !result->events.empty() &&
        co_await probe->IsServerBefore26()) {
      RescaleLegacySeverities(result->events);
    }
    co_return result;
  }

 private:
  const std::shared_ptr<scada::HistoryService> inner_;
  const std::shared_ptr<LegacySeverityProbe> probe_;
};

// Rescales the event notifications a subscription delivers. Data changes,
// item statuses and overflows pass through.
class LegacySeveritySubscription final
    : public scada::MonitoredItemSubscription {
 public:
  LegacySeveritySubscription(
      std::unique_ptr<scada::MonitoredItemSubscription> inner,
      std::shared_ptr<LegacySeverityProbe> probe)
      : inner_{std::move(inner)}, probe_{std::move(probe)} {}

  Awaitable<std::vector<scada::MonitoredItemCreateResult>> AddItems(
      std::vector<scada::MonitoredItemCreateRequest> requests) override {
    return inner_->AddItems(std::move(requests));
  }

  Awaitable<std::vector<scada::Status>> RemoveItems(
      std::span<const scada::MonitoredItemId> item_ids) override {
    return inner_->RemoveItems(item_ids);
  }

  scada::CoStatusOr<std::vector<scada::MonitoredItemNotification>> ReadNext(
      std::size_t max_count) override {
    auto probe = probe_;
    auto result = co_await inner_->ReadNext(max_count);
    if (result.ok() && HasEvents(*result) &&
        co_await probe->IsServerBefore26()) {
      for (scada::MonitoredItemNotification& notification : *result) {
        if (auto* event = std::get_if<scada::EventNotification>(&notification))
          RescaleEvent(event->event);
      }
    }
    co_return result;
  }

  void Close(scada::Status status) override {
    inner_->Close(std::move(status));
  }

 private:
  static bool HasEvents(
      const std::vector<scada::MonitoredItemNotification>& notifications) {
    return std::ranges::any_of(notifications, [](const auto& notification) {
      return std::holds_alternative<scada::EventNotification>(notification);
    });
  }

  // The event payloads a Server sends: a plain event, and a device frame,
  // whose severity sits on its base event. Anything else (a model change)
  // carries no severity.
  static void RescaleEvent(std::any& payload) {
    if (auto* event = std::any_cast<scada::Event>(&payload)) {
      event->severity = RescaleLegacySeverity(event->severity);
    } else if (auto* frame = std::any_cast<scada::DeviceFrameEvent>(&payload)) {
      frame->base.severity = RescaleLegacySeverity(frame->base.severity);
    }
  }

  const std::unique_ptr<scada::MonitoredItemSubscription> inner_;
  const std::shared_ptr<LegacySeverityProbe> probe_;
};

// Wraps every subscription it creates in `LegacySeveritySubscription`.
class LegacySeverityMonitoredItemService final
    : public scada::MonitoredItemService {
 public:
  LegacySeverityMonitoredItemService(
      std::shared_ptr<scada::MonitoredItemService> inner,
      std::shared_ptr<LegacySeverityProbe> probe)
      : inner_{std::move(inner)}, probe_{std::move(probe)} {}

  scada::StatusOr<std::unique_ptr<scada::MonitoredItemSubscription>>
  CreateSubscription(scada::ServiceContext context,
                     scada::MonitoredItemSubscriptionOptions options) override {
    auto subscription =
        inner_->CreateSubscription(std::move(context), std::move(options));
    if (!subscription.ok())
      return subscription;
    return std::unique_ptr<scada::MonitoredItemSubscription>{
        std::make_unique<LegacySeveritySubscription>(std::move(*subscription),
                                                     probe_)};
  }

 private:
  const std::shared_ptr<scada::MonitoredItemService> inner_;
  const std::shared_ptr<LegacySeverityProbe> probe_;
};

}  // namespace

DataServices RescaleLegacyEventSeverities(DataServices services) {
  auto probe =
      std::make_shared<LegacySeverityProbe>(services.attribute_service_);
  if (services.history_service_) {
    services.history_service_ = std::make_shared<LegacySeverityHistoryService>(
        std::move(services.history_service_), probe);
  }
  if (services.monitored_item_service_) {
    services.monitored_item_service_ =
        std::make_shared<LegacySeverityMonitoredItemService>(
            std::move(services.monitored_item_service_), probe);
  }
  return services;
}
