#include "services/connection_state_reporter.h"

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "client_utils.h"
#include "core/session_service.h"
#include "core/status.h"
#include "services/local_events.h"

#include <cassert>

namespace {
const base::TimeDelta kReconnectDelays[] = {base::TimeDelta::FromSeconds(1),
                                            base::TimeDelta::FromSeconds(5),
                                            base::TimeDelta::FromSeconds(30)};
}

ConnectionStateReporter::ConnectionStateReporter(
    ConnectionStateReporterContext&& context)
    : ConnectionStateReporterContext{std::move(context)} {
  session_service_.AddObserver(*this);
}

ConnectionStateReporter::~ConnectionStateReporter() {
  session_service_.RemoveObserver(*this);
}

void ConnectionStateReporter::OnSessionCreated() {
  local_events_.ReportEvent(
      LocalEvents::SEV_INFO,
      L"Связь с сервером установлена. Выполнен вход в систему.");

  reconnect_timer_.Stop();
  reconnect_retry_ = 0;
}

void ConnectionStateReporter::OnSessionDeleted(const scada::Status& status) {
  auto host_name = FormatHostName(session_service_.GetHostName());

  auto delay = reconnect_retry_ < std::size(kReconnectDelays)
                   ? kReconnectDelays[reconnect_retry_]
                   : *std::prev(std::end(kReconnectDelays));
  reconnect_retry_++;

  // User chose "Disconnect" from the menu.
  if (status) {
    local_events_.ReportEvent(
        LocalEvents::SEV_INFO,
        base::StringPrintf(L"Отключение от сервера %ls. ", host_name.c_str()));
    return;
  }

  // User logged on from another computer.
  if (status.code() == scada::StatusCode::Bad_SessionForcedLogoff) {
    local_events_.ReportEvent(
        LocalEvents::SEV_ERROR,
        base::StringPrintf(
            L"Отключение от сервера %ls. Данные реквизиты используются для "
            L"входа в систему с другого рабочего места.",
            host_name.c_str()));
    return;
  }

  local_events_.ReportEvent(
      LocalEvents::SEV_WARNING,
      base::StringPrintf(
          L"Разрыв связи с сервером %ls. %ls. Переподключение через %u секунд.",
          host_name.c_str(), ToString16(status).c_str(),
          static_cast<unsigned>(delay.InSeconds())));

  reconnect_timer_.Start(FROM_HERE, delay,
                         base::Bind(&ConnectionStateReporter::OnReconnectTimer,
                                    base::Unretained(this)));
}

void ConnectionStateReporter::OnReconnectTimer() {
  session_service_.Reconnect();
}
