#pragma once

namespace scada {
class SessionService;
}

struct DebuggerContext {
  scada::SessionService& session_service_;
};