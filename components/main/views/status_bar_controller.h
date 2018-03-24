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

struct StatusBarControllerContext {
  scada::SessionService& session_service_;
  events::EventManager& event_manager_;
  NodeService& node_service_;
};

class StatusBarController : private StatusBarControllerContext {
 public:
  explicit StatusBarController(const StatusBarControllerContext& context);

  void Init(HWND hwnd);

  void Layout();

 private:
  void Update();

  void SetPaneText(int pane, const base::string16& text);

  HWND hwnd_ = nullptr;

  base::RepeatingTimer update_timer_;
};
