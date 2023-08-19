#include "base/threading/thread_task_runner_handle.h"
#include "client_paths.h"
#include "qt/message_loop_qt.h"

#include <QApplication>

int main(int argc, char* argv[]) {
  client::RegisterPathProvider();

  QApplication qapp(argc, argv);
  base::ThreadTaskRunnerHandle message_loop{new MessageLoopQt};

  return QApplication::exec();
}
