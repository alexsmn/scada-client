#include "aui/qt/message_loop_qt.h"

#include "base/boost_log.h"
#include "base/check.h"

#include <QCoreApplication>
#include <QEvent>

#include <algorithm>
#include <exception>

namespace {

void LogUnhandledTaskException(std::exception_ptr exception) {
  try {
    std::rethrow_exception(exception);
  } catch (const std::exception& e) {
    BOOST_LOG_TRIVIAL(error) << "Unhandled exception in Qt message loop task"
                             << " | Error = " << e.what();
  } catch (...) {
    BOOST_LOG_TRIVIAL(error)
        << "Unhandled unknown exception in Qt message loop task";
  }
}

// The wakeup event type, allocated once from Qt's user range. A function-local
// static keeps the registration lazy and thread-safe without a global.
QEvent::Type WakeUpEventType() {
  static const QEvent::Type type =
      static_cast<QEvent::Type>(QEvent::registerEventType());
  return type;
}

}  // namespace

// MessageLoopQt::PendingTask

bool MessageLoopQt::PendingTask::operator<(const PendingTask& other) const {
  if (time != other.time)
    return time > other.time;  // min-heap: smaller time = higher priority
  return sequence > other.sequence;
}

// MessageLoopQt

MessageLoopQt::MessageLoopQt() {
  // postEvent() needs an application object to route through, and the wakeup is
  // the whole mechanism here — without one, tasks would queue and never run.
  scada::base::Check(QCoreApplication::instance() != nullptr,
                     "MessageLoopQt requires a QCoreApplication");

  wake_timer_.setSingleShot(true);
  QObject::connect(&wake_timer_, &QTimer::timeout, &wake_timer_,
                   [this] { Run(); });
}

MessageLoopQt::~MessageLoopQt() {
  // Order matters: refuse new wakeups first, then drop the ones already posted.
  // Reversing it leaves a window where a cross-thread post slips an event past
  // removePostedEvents() and is delivered to a destroyed object.
  shutting_down_ = true;
  wake_timer_.stop();
  QObject::disconnect(&wake_timer_, nullptr, nullptr, nullptr);
  QCoreApplication::removePostedEvents(this, WakeUpEventType());
}

void MessageLoopQt::PostDelayedTask(Clock::duration delay,
                                    Task task,
                                    const std::source_location& location) {
  scada::base::Check(task);

  {
    std::lock_guard<std::recursive_mutex> lock{mutex_};
    if (delay <= Clock::duration::zero()) {
      immediate_queue_.emplace(std::move(task));
    } else {
      delayed_queue_.emplace(PendingTask{
          .task = std::move(task),
          .time = Clock::now() + delay,
          .sequence = sequence_num_++,
      });
    }
  }

  // Wake unconditionally rather than only for immediate work: arming the timer
  // is GUI-thread-only and this may be the asio thread, so the
  // earliest-deadline decision is made in Run() where it is safe to act on.
  ScheduleWork();
}

size_t MessageLoopQt::GetTaskCount() const {
  std::lock_guard<std::recursive_mutex> lock{mutex_};
  return immediate_queue_.size() + delayed_queue_.size();
}

void MessageLoopQt::ScheduleWork() {
  if (shutting_down_)
    return;
  // Coalesce: while a wakeup is pending, a burst of a thousand posts costs one
  // event, not a thousand.
  if (wakeup_posted_.exchange(true))
    return;

  ++wakeup_count_;
  QCoreApplication::postEvent(this, new QEvent{WakeUpEventType()});
}

bool MessageLoopQt::event(QEvent* event) {
  if (event->type() == WakeUpEventType()) {
    Run();
    return true;
  }
  return QObject::event(event);
}

void MessageLoopQt::Run() {
  // Clear before touching the queues. Clearing afterwards would drop a wakeup
  // for anything posted during the drain: the flag would still read true, so no
  // event is posted, and then we clear it with work left behind.
  wakeup_posted_ = false;

  auto now = Clock::now();

  std::unique_lock<std::recursive_mutex> lock{mutex_};

  // Move due delayed tasks to the immediate queue.
  while (!delayed_queue_.empty() && delayed_queue_.top().time <= now) {
    immediate_queue_.emplace(
        std::move(const_cast<PendingTask&>(delayed_queue_.top()).task));
    delayed_queue_.pop();
  }

  // Bounded drain: run only what was queued on entry. Draining until empty lets
  // a task that re-posts itself hold the loop forever, starving paint and input
  // (and this codebase has already paid for one unbounded drain - see the
  // device-module test hang). Anything that arrives mid-pass is picked up by
  // the re-scheduled wakeup below.
  size_t budget = immediate_queue_.size();
  while (budget-- > 0 && !immediate_queue_.empty()) {
    auto task = std::move(immediate_queue_.front());
    immediate_queue_.pop();
    lock.unlock();
    try {
      task();
    } catch (...) {
      LogUnhandledTaskException(std::current_exception());
    }
    lock.lock();
  }

  const bool has_immediate_work = !immediate_queue_.empty();
  const std::optional<TimePoint> next_deadline =
      delayed_queue_.empty() ? std::nullopt
                             : std::optional{delayed_queue_.top().time};
  lock.unlock();

  if (has_immediate_work)
    ScheduleWork();
  ScheduleDelayedWork(next_deadline);
}

void MessageLoopQt::ScheduleDelayedWork(std::optional<TimePoint> deadline) {
  if (shutting_down_)
    return;

  if (!deadline) {
    wake_timer_.stop();
    armed_deadline_.reset();
    return;
  }

  if (armed_deadline_ == deadline && wake_timer_.isActive())
    return;

  // Round up. Rounding down turns a sub-millisecond delay into a 0 ms timer
  // that fires before the task is due, finds nothing, and re-arms at 0 ms -
  // a spin loop.
  const auto remaining =
      std::chrono::ceil<std::chrono::milliseconds>(*deadline - Clock::now());
  armed_deadline_ = deadline;
  wake_timer_.start(std::max<std::chrono::milliseconds>(
      remaining, std::chrono::milliseconds::zero()));
}
