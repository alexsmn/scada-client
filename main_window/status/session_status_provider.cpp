#include "main_window/status/session_status_provider.h"

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
  return connected ? u"Подключен" : u"Отключен";
}

std::u16string SessionStatusProvider::GetPingText() const {
  base::TimeDelta ping_delay;
  auto connected = session_service_.IsConnected(&ping_delay);
  return connected ? u16format(
                         L"Сервер: {} мс",
                         static_cast<unsigned>(ping_delay.InMilliseconds()))
                   : u"Нет отклика";
}
