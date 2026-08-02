#include "aui/qt/message_loop_qt.h"

#include "aui/test/app_environment.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace {

// The pump is event-driven, so a test has to let Qt deliver posted events.
// Waiting on a predicate rather than a fixed spin keeps the tests off the
// clock.
template <class Predicate>
bool PumpUntil(Predicate predicate, std::chrono::milliseconds timeout = 2s) {
  QElapsedTimer elapsed;
  elapsed.start();
  while (!predicate()) {
    if (elapsed.elapsed() >= timeout.count())
      return false;
    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    std::this_thread::sleep_for(1ms);
  }
  return true;
}

class MessageLoopQtTest : public testing::Test {
 protected:
  AppEnvironment app_env_;
  MessageLoopQt loop_;
};

TEST_F(MessageLoopQtTest, RunsAnImmediateTask) {
  bool ran = false;
  loop_.PostDelayedTask({}, [&ran] { ran = true; });

  EXPECT_TRUE(PumpUntil([&ran] { return ran; }));
}

// The point of the rewrite: a posted task is delivered by a wakeup event, so it
// runs on the next turn of the event loop rather than waiting for a tick. The
// old polled pump could not beat its own 10 ms period.
TEST_F(MessageLoopQtTest, ImmediateTaskDoesNotWaitForATimerTick) {
  QElapsedTimer elapsed;
  elapsed.start();
  bool ran = false;
  loop_.PostDelayedTask({}, [&ran] { ran = true; });

  ASSERT_TRUE(PumpUntil([&ran] { return ran; }));
  EXPECT_LT(elapsed.elapsed(), 10);
}

TEST_F(MessageLoopQtTest, RunsTasksInPostOrder) {
  std::vector<int> order;
  for (int i = 0; i < 5; ++i)
    loop_.PostDelayedTask({}, [&order, i] { order.push_back(i); });

  ASSERT_TRUE(PumpUntil([&order] { return order.size() == 5; }));
  EXPECT_EQ(order, (std::vector<int>{0, 1, 2, 3, 4}));
}

// A burst of posts must cost one wakeup, not one per task - otherwise the
// event-driven design just trades a periodic timer for an event storm.
TEST_F(MessageLoopQtTest, BurstOfPostsCoalescesIntoOneWakeup) {
  const size_t before = loop_.GetWakeupCountForTesting();
  int ran = 0;
  for (int i = 0; i < 100; ++i)
    loop_.PostDelayedTask({}, [&ran] { ++ran; });

  EXPECT_EQ(loop_.GetWakeupCountForTesting() - before, 1u);

  ASSERT_TRUE(PumpUntil([&ran] { return ran == 100; }));
}

TEST_F(MessageLoopQtTest, DelayedTasksRunInDeadlineOrder) {
  std::vector<int> order;
  loop_.PostDelayedTask(60ms, [&order] { order.push_back(3); });
  loop_.PostDelayedTask(20ms, [&order] { order.push_back(1); });
  loop_.PostDelayedTask(40ms, [&order] { order.push_back(2); });

  ASSERT_TRUE(PumpUntil([&order] { return order.size() == 3; }));
  EXPECT_EQ(order, (std::vector<int>{1, 2, 3}));
}

// The timer is armed for the earliest deadline, so a later post that is due
// sooner must retarget it instead of waiting behind the one already armed.
TEST_F(MessageLoopQtTest, EarlierDeadlineRetargetsTheArmedTimer) {
  bool late_ran = false;
  bool early_ran = false;
  loop_.PostDelayedTask(5s, [&late_ran] { late_ran = true; });
  loop_.PostDelayedTask(20ms, [&early_ran] { early_ran = true; });

  ASSERT_TRUE(PumpUntil([&early_ran] { return early_ran; }));
  EXPECT_FALSE(late_ran);
}

TEST_F(MessageLoopQtTest, DelayedTaskDoesNotRunEarly) {
  bool ran = false;
  loop_.PostDelayedTask(80ms, [&ran] { ran = true; });

  EXPECT_FALSE(PumpUntil([&ran] { return ran; }, 30ms));
  EXPECT_TRUE(PumpUntil([&ran] { return ran; }));
}

// A task that re-posts itself must not hold the pass forever: each RunOnce()
// executes only what was queued on entry. An unbounded drain here would hang
// the UI thread outright.
TEST_F(MessageLoopQtTest, SelfRepostingTaskCannotStarveThePass) {
  int runs = 0;
  std::function<void()> repost = [&] {
    ++runs;
    loop_.PostDelayedTask({}, repost);
  };
  loop_.PostDelayedTask({}, repost);

  loop_.RunOnce();
  EXPECT_EQ(runs, 1);
  loop_.RunOnce();
  EXPECT_EQ(runs, 2);
}

TEST_F(MessageLoopQtTest, TaskThrowingDoesNotStopTheLoop) {
  bool after_ran = false;
  loop_.PostDelayedTask({}, [] { throw std::runtime_error{"boom"}; });
  loop_.PostDelayedTask({}, [&after_ran] { after_ran = true; });

  EXPECT_TRUE(PumpUntil([&after_ran] { return after_ran; }));
}

// asio services the client's sockets on its own thread and posts completions
// here, so cross-thread posting is the production path, not an edge case.
TEST_F(MessageLoopQtTest, PostsFromAnotherThreadRunOnTheLoopThread) {
  const std::thread::id loop_thread = std::this_thread::get_id();
  std::atomic<bool> ran = false;
  std::thread::id ran_on;

  std::thread poster{[&] {
    loop_.PostDelayedTask({}, [&] {
      ran_on = std::this_thread::get_id();
      ran = true;
    });
  }};

  EXPECT_TRUE(PumpUntil([&ran] { return ran.load(); }));
  poster.join();
  EXPECT_EQ(ran_on, loop_thread);
}

TEST_F(MessageLoopQtTest, GetTaskCountReportsQueuedWork) {
  EXPECT_EQ(loop_.GetTaskCount(), 0u);
  loop_.PostDelayedTask({}, [] {});
  loop_.PostDelayedTask(5s, [] {});
  EXPECT_EQ(loop_.GetTaskCount(), 2u);

  loop_.RunOnce();
  EXPECT_EQ(loop_.GetTaskCount(), 1u);  // the delayed one is still pending
}

// An idle pump must not wake at all. The old design fired 100 times a second
// whether or not there was anything to do.
TEST_F(MessageLoopQtTest, IdleLoopPostsNoWakeups) {
  loop_.PostDelayedTask({}, [] {});
  ASSERT_TRUE(PumpUntil([this] { return loop_.GetTaskCount() == 0; }));

  const size_t settled = loop_.GetWakeupCountForTesting();
  PumpUntil([] { return false; }, 100ms);
  EXPECT_EQ(loop_.GetWakeupCountForTesting(), settled);
}

}  // namespace
