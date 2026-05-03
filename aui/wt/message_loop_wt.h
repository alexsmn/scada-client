#pragma once

#include "base/any_executor.h"
#include "base/any_executor_adapter.h"
#include "base/common_types.h"

#include <functional>
#include <source_location>
#include <string>

#pragma warning(push)
#pragma warning(disable : 4251 4275)
#include <wt/WTimer.h>
#pragma warning(pop)

namespace boost::asio {
class io_context;
}

class MessageLoopWt final {
 public:
  using Task = std::function<void()>;

  explicit MessageLoopWt(boost::asio::io_context& io_context);
  ~MessageLoopWt();

  void PostDelayedTask(Duration delay,
                       Task task,
                       const std::source_location& location =
                           std::source_location::current());
  size_t GetTaskCount() const;

 private:
  boost::asio::io_context& io_context_;
  std::string session_id_;
};
