#include "main_window/status_bar/session_status_provider.h"
#include "base/time_utils.h"

#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "base/format.h"
#include "base/time/time.h"
#include "base/u16format.h"
#include "base/utf_convert.h"
#include "events/local_events.h"
#include "scada/date_time.h"
#include "scada/session_service.h"

using namespace std::chrono_literals;

namespace {

// The client build version, from the module's CLIENT_BUILD_VERSION compile
// definition (set from ${PROJECT_VERSION}); empty if the module was built
// without it.
std::u16string ClientBuildLabel() {
#ifdef CLIENT_BUILD_VERSION
  return u"v" + UtfConvert<char16_t>(std::string{CLIENT_BUILD_VERSION});
#else
  return {};
#endif
}

}  // namespace

void SessionStatusProvider::Init(const ChangeNotifier& change_notifier) {
  change_notifier_ = change_notifier;

  session_poll_timer_.StartRepeating(1s, [this] { Poll(); });
}

void SessionStatusProvider::Poll() {
  const std::optional<scada::Duration> ping_delay = PingDelay();

  if (!ping_delay) {
    // No session at all: `ConnectionStateReporter` owns that message. Just
    // drop the edge state so the next stall is announced again.
    stall_reported_ = false;
  } else if (*ping_delay >= kPingStallThreshold) {
    if (!stall_reported_) {
      stall_reported_ = true;
      local_events_.ReportEvent(
          LocalEvents::SEV_WARNING,
          Translate("The server has not answered a ping for ") +
              WideFormat(static_cast<unsigned>(InMilliseconds(*ping_delay))) +
              Translate(" ms. Either the connection is degraded, or the client "
                        "itself has stopped running: the operating system can "
                        "suspend a client whose window is not visible, which "
                        "halts data and events, not only the display."));
    }
  } else if (stall_reported_) {
    stall_reported_ = false;
    local_events_.ReportEvent(LocalEvents::SEV_INFO,
                              Translate("The server is answering again."));
  }

  if (change_notifier_)
    change_notifier_();
}

std::optional<scada::Duration> SessionStatusProvider::PingDelay() const {
  scada::Duration ping_delay;
  if (!session_service_.IsConnected(&ping_delay))
    return std::nullopt;
  return ping_delay;
}

std::u16string SessionStatusProvider::GetConnectionStateText() const {
  scada::Duration ping_delay;
  auto connected = session_service_.IsConnected(&ping_delay);
  return connected ? u"\u041f\u043e\u0434\u043a\u043b\u044e\u0447\u0435\u043d"
                   : u"\u041e\u0442\u043a\u043b\u044e\u0447\u0435\u043d";
}

std::u16string SessionStatusProvider::GetPingText() const {
  const std::optional<scada::Duration> ping_delay = PingDelay();
  if (!ping_delay)
    return u"\u041d\u0435\u0442 \u043e\u0442\u043a\u043b\u0438\u043a\u0430";

  std::u16string text =
      u16format(L"\u0421\u0435\u0440\u0432\u0435\u0440: {} \u043c\u0441",
                static_cast<unsigned>(InMilliseconds(*ping_delay)));
  // The colour cue below resolves to nothing under the legacy severity theme
  // (the default), so the marker has to be in the text as well for the pane to
  // say anything an operator can read.
  if (*ping_delay >= kPingStallThreshold)
    text += u" \u00b7 " + Translate("no response");
  return text;
}

std::optional<scada::aui::Color> SessionStatusProvider::GetPingColor() const {
  const std::optional<scada::Duration> ping_delay = PingDelay();
  if (!ping_delay || *ping_delay < kPingStallThreshold)
    return std::nullopt;
  return scada::aui::SeverityColor(scada::aui::SeverityLevel::kWarning);
}

std::u16string SessionStatusProvider::GetEndpointText() const {
  const std::u16string host =
      UtfConvert<char16_t>(session_service_.GetHostName());
  const std::u16string build = ClientBuildLabel();
  if (host.empty())
    return build;
  if (build.empty())
    return host;
  return host + u" \u00b7 " + build;
}
