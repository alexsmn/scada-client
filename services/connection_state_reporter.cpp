#include "services/connection_state_reporter.h"

#include "aui/translation.h"
#include "base/u16format.h"
#include "ui/common/client_utils.h"
#include "scada/session_service.h"
#include "scada/status.h"
#include "events/local_events.h"

#include <cassert>

// Windows.h #defines ReportEvent to ReportEventA/W. Undo it.
#ifdef ReportEvent
#undef ReportEvent
#endif

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
      Translate("Connection to server established. Login successful."));

  reconnect_timer_.Stop();
  reconnect_retry_ = 0;
}

void ConnectionStateReporter::OnSessionDeleted(const scada::Status& status) {
  auto host_name = FormatHostName(session_service_.GetHostName());

  // User chose "Disconnect" from the menu.
  if (status) {
    local_events_.ReportEvent(
        LocalEvents::SEV_INFO,
        u16format(L"Disconnecting from server {}. ", host_name));
    return;
  }

  // User logged on from another computer.
  if (status.code() == scada::StatusCode::Bad_SessionForcedLogoff) {
    local_events_.ReportEvent(
        LocalEvents::SEV_ERROR,
        u16format(
            L"Disconnected from server {}. These credentials are being used to "
            L"log in from another workstation.",
            host_name));
    return;
  }

  reconnect_retry_ =
      std::min(reconnect_retry_ + 1, std::size(kReconnectDelays) - 1);

  auto delay = kReconnectDelays[reconnect_retry_];
  auto delay_s = static_cast<unsigned>(
      std::chrono::duration_cast<std::chrono::seconds>(delay).count());

  local_events_.ReportEvent(
      LocalEvents::SEV_WARNING,
      u16format(
          L"Connection to server {} lost. {}. Reconnecting in {} seconds.",
          host_name, ToString16(status), delay_s));

  reconnect_timer_.StartOne(delay, [this] { OnReconnectTimer(); });
}

void ConnectionStateReporter::OnReconnectTimer() {
  session_service_.Reconnect();
}
