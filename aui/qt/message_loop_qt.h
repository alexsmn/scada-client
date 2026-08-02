#pragma once

#include "base/any_executor.h"
#include "base/common_types.h"

#include <QObject>
#include <QTimer>
#include <atomic>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <source_location>

// The Qt client's task pump: the executor behind every `AnyExecutor` post,
// coroutine continuation and asio completion handler in the process.
//
// It is **event-driven, not polled**. Immediate work wakes the Qt event loop
// with a posted `QEvent` (`QCoreApplication::postEvent` is thread-safe and ends
// in `dispatcher->wakeUp()` — `PostMessage` on Windows, `CFRunLoopSourceSignal`
// on macOS, an eventfd write on Linux); delayed work arms a single single-shot
// `QTimer` at the *earliest* pending deadline and re-arms it as the queue
// changes. An idle loop therefore costs zero wakeups, and a posted task runs at
// the next turn of the event loop instead of waiting up to a full tick.
//
// This is the shape of Chromium's `base::MessagePump` (`ScheduleWork` /
// `ScheduleDelayedWork` over a native wakeup, one retargeted timer) rather than
// the fixed 10 ms poll this class used to be. See docs/client/message-loop.md.
//
// Threading: `PostDelayedTask` is safe from any thread — asio services the
// client's sockets on its own thread and posts completions here. Everything
// else, including construction and destruction, is GUI-thread only. A
// `QCoreApplication` must exist before the constructor runs.
class MessageLoopQt final : public QObject {
 public:
  using Task = std::function<void()>;

  MessageLoopQt();
  ~MessageLoopQt() override;

  MessageLoopQt(const MessageLoopQt&) = delete;
  MessageLoopQt& operator=(const MessageLoopQt&) = delete;

  // Queues `task` to run after `delay` (immediately when `delay` is zero) and
  // wakes the event loop. Safe to call from any thread.
  void PostDelayedTask(
      Clock::duration delay,
      Task task,
      const std::source_location& location = std::source_location::current());

  size_t GetTaskCount() const;

  // Runs one pump pass synchronously: promotes due delayed tasks and drains the
  // immediate queue. Tests use it to step the loop without a running
  // `QEventLoop`; production drives it from the posted wakeup and the timer.
  void RunOnce() { Run(); }

  // Number of wakeup events posted since construction. Immediate posts coalesce
  // into one wakeup while a pass is pending, which is the property worth
  // asserting on; exposed for tests only.
  size_t GetWakeupCountForTesting() const { return wakeup_count_; }

 private:
  struct PendingTask {
    bool operator<(const PendingTask& other) const;

    Task task;
    TimePoint time;
    int sequence = 0;
  };

  bool event(QEvent* event) override;

  // One pass: promote due delayed tasks, drain a bounded slice of the immediate
  // queue, then re-schedule whatever is left.
  void Run();

  // Asks for a `Run()` at the next turn of the event loop. Thread-safe, and
  // coalescing: repeated calls while a wakeup is already pending post nothing.
  void ScheduleWork();

  // Re-arms `wake_timer_` for `deadline`, or stops it when there is none.
  // GUI thread only.
  void ScheduleDelayedWork(std::optional<TimePoint> deadline);

  QTimer wake_timer_;
  // The deadline `wake_timer_` is currently armed for, so an unchanged head of
  // the delayed queue does not restart it on every pass.
  std::optional<TimePoint> armed_deadline_;

  // Set once destruction starts so a late cross-thread post cannot resurrect a
  // wakeup for a half-destroyed object.
  std::atomic<bool> shutting_down_ = false;
  // True while a wakeup event is posted and not yet consumed.
  std::atomic<bool> wakeup_posted_ = false;
  std::atomic<size_t> wakeup_count_ = 0;

  mutable std::recursive_mutex mutex_;
  int sequence_num_ = 0;
  std::queue<Task> immediate_queue_;
  std::priority_queue<PendingTask> delayed_queue_;
};
