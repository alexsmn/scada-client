#include "base/threading/thread_task_runner_handle.h"
#include "client_paths.h"
#include "common_resources.h"
#include "components/vidicon_display/vidicon_display_component.h"
#include "components/vidicon_display/vidicon_display_view.h"
#include "qt/message_loop_qt.h"
#include "window_definition.h"
#include "window_info.h"

#include <QApplication>
#include <QWidget>

int main(int argc, char* argv[]) {
  client::RegisterPathProvider();

  QApplication qapp(argc, argv);

  base::ThreadTaskRunnerHandle message_loop{new MessageLoopQt};

  VidiconDisplayView vidicon_display_view;

  WindowDefinition definition{kVidiconDisplayWindowInfo};
  definition.path = "pribilov.vds";
  auto* widget = vidicon_display_view.Init(definition);
  widget->resize(1000, 500);
  widget->show();

  return qapp.exec();
}