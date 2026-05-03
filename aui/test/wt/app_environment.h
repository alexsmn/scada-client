#pragma once

#include "base/any_executor.h"

#include "aui/wt/message_loop_wt.h"

#include <Wt/Test/WTestEnvironment.h>
#include <Wt/WApplication.h>
#include <boost/asio/io_context.hpp>

class AppEnvironment {
 private:
  Wt::Test::WTestEnvironment env;
  Wt::WApplication app{env};

  boost::asio::io_context io_context_;

  // Application must be created.
  AnyExecutor executor_ =
      MakeAnyExecutor(std::make_shared<MessageLoopWt>(io_context_));
};
