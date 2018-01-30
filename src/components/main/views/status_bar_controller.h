#pragma once

#include "base/strings/string16.h"
#include "base/timer/timer.h"

#include <windows.h>

namespace events {
class EventManager;
}

namespace scada {
class SessionService;
}

class NodeService;

class StatusBarController {
 public:
  StatusBarController(HWND hwnd, NodeService& node_service, events::EventManager& event_manager,
      scada::SessionService& session_service);

  void Layout();

 private:
  void Update();

  void SetPaneText(int pane, const base::string16& text);

  NodeService& node_service_;
  events::EventManager& event_manager_;
  scada::SessionService& session_service_;

  HWND hwnd_;

  base::RepeatingTimer update_timer_;
};
