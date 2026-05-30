#include "main_window/status_bar/session_status_provider.h"

#include "base/u16format.h"
#include "base/time/time.h"
#include "scada/session_service.h"

using namespace std::chrono_literals;

void SessionStatusProvider::Init(const ChangeNotifier& change_notifier) {
  change_notifier_ = change_notifier;

  session_poll_timer_.StartRepeating(1s, change_notifier_);
}

std::u16string SessionStatusProvider::GetConnectionStateText() const {
  base::TimeDelta ping_delay;
  auto connected = session_service_.IsConnected(&ping_delay);
  return connected ? u"\u041f\u043e\u0434\u043a\u043b\u044e\u0447\u0435\u043d" : u"\u041e\u0442\u043a\u043b\u044e\u0447\u0435\u043d";
}

std::u16string SessionStatusProvider::GetPingText() const {
  base::TimeDelta ping_delay;
  auto connected = session_service_.IsConnected(&ping_delay);
  return connected ? u16format(
                         L"\u0421\u0435\u0440\u0432\u0435\u0440: {} \u043c\u0441",
                         static_cast<unsigned>(ping_delay.InMilliseconds()))
                   : u"\u041d\u0435\u0442 \u043e\u0442\u043a\u043b\u0438\u043a\u0430";
}
