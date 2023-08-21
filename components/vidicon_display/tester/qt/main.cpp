#include "base/test/test_executor.h"
#include "base/threading/thread_task_runner_handle.h"
#include "client_paths.h"
#include "components/vidicon_display/vidicon_display_component.h"
#include "components/vidicon_display/vidicon_display_view.h"
#include "components/vidicon_display/vidicon_display_view2.h"
#include "qt/message_loop_qt.h"
#include "services/vidicon/vidicon_client.h"
#include "test/display_tester/qt/display_tester_state.h"
#include "test/display_tester/qt/display_tester_window.h"
#include "window_definition.h"

#include <QApplication>

struct State : DisplayTesterState {
  std::shared_ptr<Executor> executor = std::make_shared<TestExecutor>();
  vidicon::VidiconClient vidicon_client{
      {.executor_ = executor, .timed_data_service_ = timed_data_service}};
};

QWidget* CreateVidiconDisplayView(State& state,
                                  const std::filesystem::path& path) {
  /* VidiconDisplayView* vidicon_display_view =
      new VidiconDisplayView{state.vidicon_client};*/
  VidiconDisplayView2* vidicon_display_view =
      new VidiconDisplayView2{{state.timed_data_service, state.vidicon_client,
                               state.controller_delegate}};

  WindowDefinition definition{kVidiconDisplayWindowInfo};
  definition.path =
      !path.empty()
          ? path
          : std::filesystem::path{
                R"(c:\ProgramData\Telecontrol\SCADA Client\по-258.vds)"};

  return vidicon_display_view->Init(definition);
}

int main(int argc, char* argv[]) {
  client::RegisterPathProvider();

  QApplication qapp(argc, argv);
  base::ThreadTaskRunnerHandle message_loop{new MessageLoopQt};
  State state;

  DisplayTesterWindow tester_window{
      state, std::bind_front(&CreateVidiconDisplayView, std::ref(state))};

  tester_window.show();

  return QApplication::exec();
}