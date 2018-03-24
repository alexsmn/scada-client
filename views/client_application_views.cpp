#include "views/client_application_views.h"

#include "base/run_loop.h"
#include "components/login/login_dialog.h"
#include "components/main/views/main_window_views.h"
#include "ui/views/focus/accelerator_handler.h"

// ClientApplicationViews

ClientApplicationViews* g_application_views = nullptr;

ClientApplicationViews::ClientApplicationViews(int argc, char** argv)
    : ClientApplication(argc, argv) {
}

void ClientApplicationViews::AddMessageDispatcher(base::MessagePumpDispatcher& dispatcher) {
  message_dispatchers_.AddObserver(&dispatcher);
}

void ClientApplicationViews::RemoveMessageDispatcher(base::MessagePumpDispatcher& dispatcher) {
  message_dispatchers_.RemoveObserver(&dispatcher);
}

int ClientApplicationViews::Run(int show) {
  views::AcceleratorHandler accelerator_handler;
  AddMessageDispatcher(accelerator_handler);

  base::RunLoop run_loop(this);
  loop_ = &run_loop;
  run_loop.Run();
  loop_ = nullptr;

  RemoveMessageDispatcher(accelerator_handler);

  return 0;
}

void ClientApplicationViews::Quit() {
  if (loop_)
    loop_->Quit();
}

uint32_t ClientApplicationViews::Dispatch(const base::NativeEvent& event) {
  if (message_dispatchers_.might_have_observers()) {
    for (auto& dispatcher : message_dispatchers_) {
      auto result = dispatcher.Dispatch(event);
      if (result != POST_DISPATCH_PERFORM_DEFAULT)
        return result;
    }
  }
  return POST_DISPATCH_PERFORM_DEFAULT;
}

std::unique_ptr<MainWindow> ClientApplicationViews::CreateMainWindow(MainWindowContext&& context) {
  return std::make_unique<MainWindowViews>(std::move(context));
}

bool ClientApplicationViews::ShowLoginDialogImpl(const DataServicesContext& context, DataServices& services) {
  return ExecuteLoginDialog(context, services);
}
