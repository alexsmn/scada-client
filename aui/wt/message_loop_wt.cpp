#include "aui/wt/message_loop_wt.h"

#pragma warning(push)
#pragma warning(disable : 4251 4275)
#include <wt/WServer.h>
#pragma warning(pop)

using namespace std::chrono_literals;

MessageLoopWt::MessageLoopWt(boost::asio::io_context& io_context)
    : io_context_{io_context},
      session_id_{Wt::WApplication::instance()->sessionId()} {
  Wt::WApplication::instance()->enableUpdates(true);
  /*timer_.setInterval(10ms);
  timer_.timeout().connect([this] { Run(); });
  timer_.start();*/
}

MessageLoopWt::~MessageLoopWt() {}

void MessageLoopWt::Run() {
  auto ticks = base::TimeTicks::Now();

  std::unique_lock<std::recursive_mutex> lock{mutex_};
  while (!queue_.empty() && queue_.top().delayed_run_time <= ticks) {
    auto pending_task = std::move(const_cast<base::PendingTask&>(queue_.top()));
    queue_.pop();
    lock.unlock();
    std::move(pending_task.task).Run();
    lock.lock();
  }
}

bool MessageLoopWt::PostTaskHelper(const base::Location& from_here,
                                   base::OnceClosure task,
                                   base::TimeDelta delay,
                                   base::Nestable nestable) {
  assert(task);

  /*auto ticks = base::TimeTicks::Now() + delay;

  std::lock_guard<std::recursive_mutex> lock{mutex_};
  base::PendingTask pending_task(from_here, std::move(task), ticks, nestable);
  pending_task.sequence_num = sequence_num_++;
  queue_.emplace(std::move(pending_task));*/

  Wt::WServer::instance()->schedule(
      std::chrono::microseconds{delay.InMicroseconds()}, session_id_,
      [task_ptr = std::make_shared<base::OnceClosure>(std::move(task))] {
        std::move(*task_ptr).Run();
        Wt::WApplication::instance()->triggerUpdate();
      });

  return true;
}
