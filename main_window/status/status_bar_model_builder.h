#pragma once

#include <memory>

namespace aui {
class StatusBarModel;
}

namespace scada {
class SessionService;
}

class Executor;
class NodeEventProvider;
class NodeService;
class Profile;
class StatusProvider;

struct StatusBarModelBuilder {
  std::shared_ptr<aui::StatusBarModel> Build();

  std::shared_ptr<Executor> executor_;
  scada::SessionService& session_service_;
  NodeEventProvider& node_event_provider_;
  NodeService& node_service_;
  Profile& profile_;
};