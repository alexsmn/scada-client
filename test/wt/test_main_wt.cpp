#include "base/boost_log_init.h"
#include "base/file_path_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "wt/message_loop_wt.h"

#include <Wt/Test/WTestEnvironment.h>
#include <Wt/WApplication.h>
#include <base/files/file_path.h>
#include <base/path_service.h>
#include <boost/asio/io_context.hpp>
#include <gmock/gmock.h>

int main(int argc, char** argv) {
  {
    base::FilePath path;
    if (!base::PathService::Get(base::DIR_EXE, &path))
      return 1;
    path = path.AppendASCII("logs");
    InitBoostLogging({.path = AsFilesystemPath(path), .console = true});
  }

  Wt::Test::WTestEnvironment env;
  Wt::WApplication app{env};

  // QApplication must be created.
  boost::asio::io_context io_context;
  auto task_runner = base::MakeRefCounted<MessageLoopWt>(io_context);
  base::ThreadTaskRunnerHandle message_loop{task_runner};

  ::testing::InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}