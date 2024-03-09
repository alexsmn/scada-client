#pragma once

#include <boost/signals2/connection.hpp>
#include <functional>
#include <string>

class NodeEventProvider;
class Profile;

class EventStatusProvider final {
 public:
  using ChangeNotifier = std::function<void()>;

  EventStatusProvider(NodeEventProvider& node_event_provider, Profile& profile)
      : node_event_provider_{node_event_provider}, profile_{profile} {}

  void Init(const ChangeNotifier& change_notifier);

  std::u16string GetEventCountText() const;
  std::u16string GetSeverityText() const;

 private:
  NodeEventProvider& node_event_provider_;
  Profile& profile_;

  ChangeNotifier change_notifier_;

  boost::signals2::scoped_connection connection_;
};
