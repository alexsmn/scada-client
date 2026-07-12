#pragma once

#include "aui/severity_colors.h"
#include "events/event_observer.h"

#include <boost/signals2/connection.hpp>
#include <functional>
#include <optional>
#include <string>

class LocalEvents;
class NodeEventProvider;
class Profile;

// Buckets a raw OPC UA event severity (0-1000) into a coarse alarm level:
// >= critical threshold => kCritical, >= warning threshold => kWarning, else
// kNone. Pure, shared by the highest-severity indicator and the KPI counts.
aui::SeverityLevel SeverityLevelForEvent(unsigned severity);

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

  // Number of currently unacknowledged alarms, for an unread badge.
  int GetAlarmCount() const;

  // Number of currently unacknowledged alarms at a given severity level, for
  // the live severity KPI tiles.
  int GetSeverityCount(aui::SeverityLevel level) const;

  // The worst active (unacknowledged) alarm shown as a coloured indicator. Both
  // are empty/none under the legacy theme, so the default status bar is
  // unchanged; under the opt-in token themes they surface the highest severity
  // and its colour (from the severity single source).
  std::u16string GetHighestSeverityText() const;
  std::optional<aui::Color> GetHighestSeverityColor() const;

 private:
  // Highest severity among the currently unacknowledged alarms, or kNone.
  aui::SeverityLevel HighestUnackedLevel() const;

  // EventObserver
  void OnEvents(std::span<const scada::Event* const> events) override;

  NodeEventProvider& node_event_provider_;
  LocalEvents& local_events_;
  Profile& profile_;

  ChangeNotifier change_notifier_;

  std::vector<boost::signals2::scoped_connection> connections_;
};
