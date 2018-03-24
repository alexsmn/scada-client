#pragma once

#include "base/message_loop/message_pump_dispatcher.h"
#include "base/observer_list.h"
#include "ui/views/message_dispatch_controller.h"

class ActiveXHost : public views::MessageDispatchController,
                    public base::MessagePumpDispatcher {
 public:
  // views::MessageDispatchController
  virtual void AddMessageDispatcher(
      base::MessagePumpDispatcher& dispatcher) override;
  virtual void RemoveMessageDispatcher(
      base::MessagePumpDispatcher& dispatcher) override;

  static ActiveXHost& instance();

 private:
  // base::MessagePumpDispatcher
  virtual uint32_t Dispatch(const base::NativeEvent& event) override;

  base::ObserverList<base::MessagePumpDispatcher> message_dispatchers_;
};
