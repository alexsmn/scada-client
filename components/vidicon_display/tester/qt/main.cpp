#include <QApplication>

#include "base/threading/thread_task_runner_handle.h"
#include "client_paths.h"
#include "common_resources.h"
#include "components/vidicon_display/qt/vidicon_display_view.h"
#include "qt/message_loop_qt.h"
#include "window_definition.h"
#include "window_info.h"

int main(int argc, char* argv[]) {
  client::RegisterPathProvider();

  QApplication qapp(argc, argv);

  base::ThreadTaskRunnerHandle message_loop{new MessageLoopQt};

  VidiconDisplayView vidicon_display_view;

  WindowDefinition definition{GetWindowInfo(ID_VIDICON_DISPLAY_VIEW)};
  definition.path = base::FilePath{FILE_PATH_LITERAL("pribilov.vds")};
  auto* widget = vidicon_display_view.Init(definition);
  widget->resize(1000, 500);
  widget->show();

  return qapp.exec();
}