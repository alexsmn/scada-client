#pragma once

#include "aui/wt/message_loop_wt.h"
#include "base/threading/thread_task_runner_handle.h"

#include <Wt/Test/WTestEnvironment.h>
#include <Wt/WApplication.h>
#include <boost/asio/io_context.hpp>

class AppEnvironment {
 private:
  Wt::Test::WTestEnvironment env;
  Wt::WApplication app{env};

  boost::asio::io_context io_context_;

  // Application must be created.
  base::ThreadTaskRunnerHandle task_runner_handle_{
      base::MakeRefCounted<MessageLoopWt>(io_context_)};
};
