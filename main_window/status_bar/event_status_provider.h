#pragma once

#include "aui/severity_colors.h"
#include "events/event_observer.h"
#include "events/event_severity.h"
#include "events/severity_tiles.h"

#include <boost/signals2/connection.hpp>
#include <functional>
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

  // The events panel's minimum-severity filter, as a reminder that the panel
  // hides rows ("Panel filter: severity >= N"). Empty when the panel shows
  // every event, so the status strip says nothing about an unset filter.
  // Alarm counts are not here: they live once, in the top bar's tiles
  // (principles §8).
  std::u16string GetPanelFilterText() const;

  // The KPI severity tile counts for the current alarm set — the single
  // aggregation the accessors below are views onto.
  events::SeverityTileCounts GetTileCounts() const;

  // Number of currently unacknowledged alarms, for an unread badge.
  int GetAlarmCount() const;

  // Number of currently unacknowledged alarms at a given severity level, for
  // the live severity KPI tiles.
  int GetSeverityCount(scada::aui::SeverityLevel level) const;

  // Number of critical alarms nobody has acknowledged, standing or cleared,
  // for the escalation ladder's annunciator (backlog 1117).
  int GetUnackedCriticalCount() const;

 private:
  // EventObserver
  void OnEvents(std::span<const scada::Event* const> events) override;

  NodeEventProvider& node_event_provider_;
  LocalEvents& local_events_;
  Profile& profile_;

  ChangeNotifier change_notifier_;

  std::vector<boost::signals2::scoped_connection> connections_;
};
