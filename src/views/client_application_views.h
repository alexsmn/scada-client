#pragma once

#include "base/message_loop/message_pump_dispatcher.h"
#include "base/observer_list.h"
#include "client/client_application.h"
#include "ui/views/message_dispatch_controller.h"

namespace base {
class RunLoop;
}

class ClientApplicationViews : public ClientApplication,
                               public views::MessageDispatchController,
                               private base::MessagePumpDispatcher {
 public:
  ClientApplicationViews(int argc, char** argv);

  // views::MessageDispatchController
  virtual void AddMessageDispatcher(base::MessagePumpDispatcher& dispatcher) override;
  virtual void RemoveMessageDispatcher(base::MessagePumpDispatcher& dispatcher) override;

  // ClientApplication
  virtual bool ShowLoginDialog() override;
  virtual int Run(int show) override;
  virtual void Quit() override;

 protected:
  // ClientApplication
  virtual std::unique_ptr<MainWindow> CreateMainWindow(MainWindowContext&& context) override;

 private:
  // base::MessagePumpDispatcher
  virtual uint32_t Dispatch(const base::NativeEvent& event) override;

  base::ObserverList<base::MessagePumpDispatcher> message_dispatchers_;

  base::RunLoop* loop_ = nullptr;
};

extern ClientApplicationViews* g_application_views;