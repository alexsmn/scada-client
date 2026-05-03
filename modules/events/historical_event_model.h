#pragma once

#include "base/any_executor.h"

#include "base/awaitable.h"
#include "base/boost_log.h"
#include "base/format_time.h"
#include "base/cancelation.h"
#include "base/time_range.h"
#include "scada/event.h"
#include "scada/coroutine_services.h"
#include "scada/history_service.h"

#include <boost/signals2/signal.hpp>
#include <list>


class HistoricalEventModel {
 public:
  HistoricalEventModel(AnyExecutor executor,
                       scada::HistoryService& history_service)
      : executor_{std::move(executor)},
        history_service_{history_service} {}

  void Init(const TimeRange& range) { time_range_ = range; }

  const TimeRange& time_range() const { return time_range_; }
  void SetTimeRange(const TimeRange& range) { time_range_ = range; }

  const auto& events() const { return historical_events_; }

  const scada::Event& AddEvent(scada::Event event) {
    return historical_events_.emplace_back(std::move(event));
  }

  void Update();

  bool working() const { return request_running_; }

  void CancelRequest() {
    cancelation_.Cancel();
    request_running_ = false;
  }

  boost::signals2::signal<void()> refilter_now;

 private:
  Awaitable<void> UpdateAsync(CancelationRef request_cancelation,
                              base::Time from,
                              base::Time to);

  void OnHistoryReadEventsCompleted(scada::HistoryReadEventsResult&& result);

  const AnyExecutor executor_;

  scada::HistoryService& history_service_;

  // Filter.
  TimeRange time_range_;

  // Contains only historical events. |rows_| holds pointers on items from
  // it, so it shall not be vector.
  using EventContainer = std::list<scada::Event>;
  EventContainer historical_events_;

  bool request_running_ = false;

  Cancelation cancelation_;
};

inline void HistoricalEventModel::Update() {
  historical_events_.clear();

  auto [from, to] = ToDateTimeRange(time_range_, /*now=*/base::Time::Now());

  BOOST_LOG_TRIVIAL(info) << "Query events from " << FormatTime(from).c_str();

  assert(!request_running_);
  request_running_ = true;

  CoSpawn(executor_,
          [this, request_cancelation = cancelation_.ref(), from,
           to]() mutable -> Awaitable<void> {
            if (request_cancelation.canceled()) {
              co_return;
            }

            co_await UpdateAsync(request_cancelation, from, to);
          });
}

inline Awaitable<void> HistoricalEventModel::UpdateAsync(
    CancelationRef request_cancelation,
    base::Time from,
    base::Time to) {
  auto result = co_await history_service_.HistoryReadEvents(
      scada::id::Server, from, to,
      scada::EventFilter{scada::EventFilter::ACKED});
  if (request_cancelation.canceled()) {
    co_return;
  }

  OnHistoryReadEventsCompleted(std::move(result));
}

inline void HistoricalEventModel::OnHistoryReadEventsCompleted(
    scada::HistoryReadEventsResult&& result) {
  assert(request_running_);
  // Only acked events were requested.
  assert(std::ranges::all_of(
      result.events, [](const scada::Event& event) { return event.acked; }));

  historical_events_.assign(std::make_move_iterator(result.events.begin()),
                            std::make_move_iterator(result.events.end()));

  request_running_ = false;

  refilter_now();
}
