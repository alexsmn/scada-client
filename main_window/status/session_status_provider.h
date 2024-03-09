#pragma once

#include "base/executor_timer.h"

#include <functional>
#include <string>

namespace scada {
class SessionService;
}

class SessionStatusProvider {
 public:
  using ChangeNotifier = std::function<void()>;

  SessionStatusProvider(const std::shared_ptr<Executor> executor,
                        scada::SessionService& session_service)
      : session_service_{session_service}, session_poll_timer_{executor} {}

  void Init(const ChangeNotifier& change_notifier);

  std::u16string GetConnectionStateText() const;
  std::u16string GetPingText() const;

 private:
  scada::SessionService& session_service_;

  ChangeNotifier change_notifier_;

  ExecutorTimer session_poll_timer_;
};