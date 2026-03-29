#pragma once

#include "base/executor.h"

#include <string>

#pragma warning(push)
#pragma warning(disable : 4251 4275)
#include <wt/WTimer.h>
#pragma warning(pop)

namespace boost::asio {
class io_context;
}

class MessageLoopWt final : public Executor {
 public:
  explicit MessageLoopWt(boost::asio::io_context& io_context);
  ~MessageLoopWt();

  // Executor
  virtual void PostDelayedTask(Duration delay,
                               Task task,
                               const std::source_location& location =
                                   std::source_location::current()) override;
  virtual size_t GetTaskCount() const override;

 private:
  boost::asio::io_context& io_context_;
  std::string session_id_;
};
