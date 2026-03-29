#include "aui/wt/message_loop_wt.h"

#pragma warning(push)
#pragma warning(disable : 4251 4275)
#include <wt/WServer.h>
#pragma warning(pop)

#include <cassert>

MessageLoopWt::MessageLoopWt(boost::asio::io_context& io_context)
    : io_context_{io_context},
      session_id_{Wt::WApplication::instance()->sessionId()} {
  Wt::WApplication::instance()->enableUpdates(true);
}

MessageLoopWt::~MessageLoopWt() {}

void MessageLoopWt::PostDelayedTask(Duration delay,
                                    Task task,
                                    const std::source_location& location) {
  assert(task);

  Wt::WServer::instance()->schedule(
      std::chrono::duration_cast<std::chrono::milliseconds>(delay),
      session_id_,
      [task = std::make_shared<Task>(std::move(task))] {
        (*task)();
        Wt::WApplication::instance()->triggerUpdate();
      });
}

size_t MessageLoopWt::GetTaskCount() const {
  return 0;
}
