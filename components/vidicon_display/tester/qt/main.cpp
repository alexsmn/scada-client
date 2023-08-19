#include "base/test/test_executor.h"
#include "base/threading/thread_task_runner_handle.h"
#include "client_paths.h"
#include "common_resources.h"
#include "components/vidicon_display/tester/qt/tester_state.h"
#include "components/vidicon_display/tester/qt/tester_window.h"
#include "components/vidicon_display/vidicon_display_component.h"
#include "components/vidicon_display/vidicon_display_view.h"
#include "components/vidicon_display/vidicon_display_view2.h"
#include "qt/message_loop_qt.h"
#include "services/vidicon/vidicon_client.h"
#include "window_definition.h"
#include "window_info.h"

#include <QApplication>
#include <QFileDialog>

struct State : TesterState {
  std::shared_ptr<Executor> executor = std::make_shared<TestExecutor>();
  vidicon::VidiconClient vidicon_client{
      {.executor_ = executor, .timed_data_service_ = timed_data_service}};
};

QWidget* CreateVidiconDisplayView(QSplitter& splitter,
                                  State& state,
                                  const std::filesystem::path& path) {
  /* VidiconDisplayView* vidicon_display_view =
      new VidiconDisplayView{state.vidicon_client};*/
  VidiconDisplayView2* vidicon_display_view =
      new VidiconDisplayView2{{state.timed_data_service, state.vidicon_client,
                               state.controller_delegate}};
  WindowDefinition definition{kVidiconDisplayWindowInfo};
  // definition.path = R"(c:\ProgramData\Telecontrol\SCADA Client\PS-110.vds)";
  definition.path = path;
  auto* widget = vidicon_display_view->Init(definition);
  widget->setParent(&splitter);
  QObject::connect(widget, &QObject::destroyed,
                   [vidicon_display_view] { delete vidicon_display_view; });
  splitter.addWidget(widget);
  return widget;
}

int main(int argc, char* argv[]) {
  client::RegisterPathProvider();

  QApplication qapp(argc, argv);
  base::ThreadTaskRunnerHandle message_loop{new MessageLoopQt};
  State state;

  TesterWindow tester_window{state};

  QWidget* opened_vidicon_display_view = nullptr;

  tester_window.toolbar->addAction("Open Default", [&] {
    opened_vidicon_display_view = CreateVidiconDisplayView(
        *tester_window.splitter, state,
        R"(c:\ProgramData\Telecontrol\SCADA Client\по-258.vds)");
  });

  tester_window.toolbar->addAction("Open...", [&] {
    if (auto path = QFileDialog::getOpenFileName(&tester_window);
        !path.isEmpty()) {
      opened_vidicon_display_view = CreateVidiconDisplayView(
          *tester_window.splitter, state, path.toStdWString());
    }
  });

  tester_window.toolbar->addAction("Close", [&opened_vidicon_display_view] {
    if (opened_vidicon_display_view) {
      opened_vidicon_display_view->deleteLater();
      opened_vidicon_display_view = nullptr;
    }
  });

  tester_window.show();

  return QApplication::exec();
}