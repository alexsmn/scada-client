#include "services/connection_state_reporter.h"

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "client_utils.h"
#include "core/session_service.h"
#include "core/status.h"
#include "services/local_events.h"

#include <cassert>

using namespace std::chrono_literals;

namespace {
const Duration kReconnectDelays[] = {1s, 5s, 30s};
}

ConnectionStateReporter::ConnectionStateReporter(
    ConnectionStateReporterContext&& context)
    : ConnectionStateReporterContext{std::move(context)},
      session_state_changed_connection_{
          session_service_.SubscribeSessionStateChanged(
              BindExecutor(executor_,
                           [this](bool connected, const scada::Status& status) {
                             if (connected)
                               OnSessionCreated();
                             else
                               OnSessionDeleted(status);
                           }))} {}

ConnectionStateReporter::~ConnectionStateReporter() {}

void ConnectionStateReporter::OnSessionCreated() {
  local_events_.ReportEvent(
      LocalEvents::SEV_INFO,
      u"Связь с сервером установлена. Выполнен вход в систему.");

  reconnect_timer_.Stop();
  reconnect_retry_ = 0;
}

void ConnectionStateReporter::OnSessionDeleted(const scada::Status& status) {
  auto host_name = FormatHostName(session_service_.GetHostName());

  // User chose "Disconnect" from the menu.
  if (status) {
    local_events_.ReportEvent(
        LocalEvents::SEV_INFO,
        base::StringPrintf(u"Отключение от сервера %ls. ", host_name.c_str()));
    return;
  }

  // User logged on from another computer.
  if (status.code() == scada::StatusCode::Bad_SessionForcedLogoff) {
    local_events_.ReportEvent(
        LocalEvents::SEV_ERROR,
        base::StringPrintf(
            u"Отключение от сервера %ls. Данные реквизиты используются для "
            u"входа в систему с другого рабочего места.",
            host_name.c_str()));
    return;
  }

  reconnect_retry_ =
      std::min(reconnect_retry_ + 1, std::size(kReconnectDelays) - 1);

  auto delay = kReconnectDelays[reconnect_retry_];
  auto delay_s = static_cast<unsigned>(
      std::chrono::duration_cast<std::chrono::seconds>(delay).count());

  local_events_.ReportEvent(
      LocalEvents::SEV_WARNING,
      base::StringPrintf(
          u"Разрыв связи с сервером %ls. %ls. Переподключение через %u секунд.",
          host_name.c_str(), ToString16(status).c_str(), delay_s));

  reconnect_timer_.StartOne(delay, [this] { OnReconnectTimer(); });
}

void ConnectionStateReporter::OnReconnectTimer() {
  session_service_.Reconnect();
}
