#pragma once

#include "base/format_time.h"
#include "base/memory/weak_ptr.h"
#include "scada/event.h"
#include "scada/history_service.h"
#include "base/time_range.h"

#include <boost/signals2/signal.hpp>
#include <list>

class Executor;

class HistoricalEventModel {
 public:
  HistoricalEventModel(std::shared_ptr<Executor> executor,
                       scada::HistoryService& history_service)
      : executor_{std::move(executor)}, history_service_{history_service} {}

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
    weak_factory_.InvalidateWeakPtrs();
    request_running_ = false;
  }

  boost::signals2::signal<void()> refilter_now;

 private:
  void OnHistoryReadEventsCompleted(scada::Status&& status,
                                    std::vector<scada::Event>&& events);

  // The current executor used to sync `history_service_` responses.
  const std::shared_ptr<Executor> executor_;

  scada::HistoryService& history_service_;

  // Filter.
  TimeRange time_range_;

  // Contains only historical events. |rows_| holds pointers on items from
  // it, so it shall not be vector.
  using EventContainer = std::list<scada::Event>;
  EventContainer historical_events_;

  bool request_running_ = false;

  base::WeakPtrFactory<HistoricalEventModel> weak_factory_{this};
};

inline void HistoricalEventModel::Update() {
  historical_events_.clear();

  auto [from, to] = GetTimeRangeBounds(time_range_);

  LOG(INFO) << "Query events from " << FormatTime(from).c_str();

  assert(!request_running_);
  request_running_ = true;

  auto weak_ptr = weak_factory_.GetWeakPtr();
  history_service_.HistoryReadEvents(
      scada::id::Server, from, to,
      scada::EventFilter{scada::EventFilter::ACKED},
      BindExecutor(executor_, [this, weak_ptr](
                                  scada::Status status,
                                  std::vector<scada::Event> events) {
        if (weak_ptr.get())
          OnHistoryReadEventsCompleted(std::move(status), std::move(events));
      }));
}

inline void HistoricalEventModel::OnHistoryReadEventsCompleted(
    scada::Status&& status,
    std::vector<scada::Event>&& events) {
  assert(request_running_);
  // Only acked events were requested.
  assert(std::all_of(events.begin(), events.end(),
                     [](const scada::Event& event) { return event.acked; }));

  historical_events_.assign(std::make_move_iterator(events.begin()),
                            std::make_move_iterator(events.end()));

  request_running_ = false;

  refilter_now();
}
