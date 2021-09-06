#pragma once

#include "base/pending_task.h"
#include "base/single_thread_task_runner.h"

#include <mutex>

#pragma warning(push)
#pragma warning(disable : 4251 4275)
#include <wt/WTimer.h>
#pragma warning(pop)

namespace boost::asio {
class io_context;
}

class MessageLoopWt final : public base::SingleThreadTaskRunner {
 public:
  explicit MessageLoopWt(boost::asio::io_context& io_context);
  ~MessageLoopWt();

  void Run();

  virtual bool PostDelayedTask(const base::Location& from_here,
                               base::OnceClosure task,
                               base::TimeDelta delay) override {
    return PostTaskHelper(from_here, std::move(task), delay,
                          base::Nestable::kNestable);
  }

  virtual bool PostNonNestableDelayedTask(const base::Location& from_here,
                                          base::OnceClosure task,
                                          base::TimeDelta delay) override {
    return PostTaskHelper(from_here, std::move(task), delay,
                          base::Nestable::kNonNestable);
  }

  virtual bool RunsTasksInCurrentSequence() const override { return true; }

 private:
  bool PostTaskHelper(const base::Location& from_here,
                      base::OnceClosure task,
                      base::TimeDelta delay,
                      base::Nestable nestable);

  boost::asio::io_context& io_context_;

  std::recursive_mutex mutex_;
  int sequence_num_ = 0;
  base::DelayedTaskQueue queue_;

  //Wt::WTimer timer_;
  std::string session_id_;
};
