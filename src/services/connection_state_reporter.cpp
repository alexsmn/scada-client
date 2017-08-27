#include "services/connection_state_reporter.h"

#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "core/status.h"
#include "core/session_service.h"
#include "services/local_events.h"

#include <cassert>

ConnectionStateReporter::ConnectionStateReporter(scada::SessionService& session_state, LocalEvents& local_events)
    : session_service_(session_state),
      local_events_(local_events) {
  session_service_.AddObserver(*this);
}

ConnectionStateReporter::~ConnectionStateReporter() {
  session_service_.RemoveObserver(*this);
}

void ConnectionStateReporter::OnSessionCreated() {
  local_events_.ReportEvent(LocalEvents::SEV_INFO,
      L"Связь с сервером восстановлена. Выполнен повторный вход в систему");
}

void ConnectionStateReporter::OnSessionDeleted(const scada::Status& status) {
  base::string16 message = (status.code() == scada::StatusCode::Bad_SessionForcedLogoff) ?
      L"Разрыв связи со стороны сервера" :
      base::StringPrintf(L"Связь с сервером разорвана из-за ошибки (%ls)",
          status.ToString16().c_str());
  local_events_.ReportEvent(LocalEvents::SEV_ERROR, message);
}
