#pragma once

#include "aui/severity_colors.h"
#include "events/event_observer.h"
#include "events/event_severity.h"
#include "events/severity_tiles.h"

#include <boost/signals2/connection.hpp>
#include <functional>
#include <optional>
#include <string>

class LocalEvents;
class NodeEventProvider;
class Profile;

class EventStatusProvider final : private EventObserver {
 public:
  using ChangeNotifier = std::function<void()>;

  EventStatusProvider(NodeEventProvider& node_event_provider,
                      LocalEvents& local_events,
                      Profile& profile)
      : node_event_provider_{node_event_provider},
        local_events_{local_events},
        profile_{profile} {}

  ~EventStatusProvider();

  void Init(const ChangeNotifier& change_notifier);

  std::u16string GetEventCountText() const;
  std::u16string GetSeverityText() const;

  // The KPI severity tile counts for the current alarm set — the single
  // aggregation the accessors below are views onto.
  events::SeverityTileCounts GetTileCounts() const;

  // Number of currently unacknowledged alarms, for an unread badge.
  int GetAlarmCount() const;

  // Number of currently unacknowledged alarms at a given severity level, for
  // the live severity KPI tiles.
  int GetSeverityCount(scada::aui::SeverityLevel level) const;

  // The worst active (unacknowledged) alarm, as a label and a colour from the
  // severity single source. Both are empty when nothing is asserted — a calm
  // status strip says nothing rather than saying "normal".
  std::u16string GetHighestSeverityText() const;
  std::optional<scada::aui::Color> GetHighestSeverityColor() const;

 private:
  // Highest severity among the currently unacknowledged alarms, or kNone.
  scada::aui::SeverityLevel HighestUnackedLevel() const;

  // EventObserver
  void OnEvents(std::span<const scada::Event* const> events) override;

  NodeEventProvider& node_event_provider_;
  LocalEvents& local_events_;
  Profile& profile_;

  ChangeNotifier change_notifier_;

  std::vector<boost::signals2::scoped_connection> connections_;
};
