#include "views/activex_host.h"

// static
ActiveXHost& ActiveXHost::instance() {
  static ActiveXHost s_instance;
  return s_instance;
}

void ActiveXHost::AddMessageDispatcher(
    base::MessagePumpDispatcher& dispatcher) {
  message_dispatchers_.AddObserver(&dispatcher);
}

void ActiveXHost::RemoveMessageDispatcher(
    base::MessagePumpDispatcher& dispatcher) {
  message_dispatchers_.RemoveObserver(&dispatcher);
}

uint32_t ActiveXHost::Dispatch(const base::NativeEvent& event) {
  if (message_dispatchers_.might_have_observers()) {
    for (auto& dispatcher : message_dispatchers_) {
      auto result = dispatcher.Dispatch(event);
      if (result != POST_DISPATCH_PERFORM_DEFAULT)
        return result;
    }
  }
  return POST_DISPATCH_PERFORM_DEFAULT;
}
