#include "settle_loop.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

namespace scada::screenshot_generator {

namespace {

// A frame source that changes for the first `changing` grabs and is constant
// afterwards, so a test can say exactly when the widget "stops repainting".
class FrameSource {
 public:
  explicit FrameSource(int changing) : changing_(changing) {}

  int operator()() {
    ++grabs_;
    return grabs_ <= changing_ ? grabs_ : 0;
  }

  int grabs() const { return grabs_; }

 private:
  const int changing_;
  int grabs_ = 0;
};

}  // namespace

TEST(SettleLoopTest, SettlesOnceTheFrameStopsChanging) {
  FrameSource frames{2};
  int pumps = 0;

  const SettleResult result =
      RunSettleLoop(std::ref(frames), [&] { ++pumps; }, /*max_frames=*/20);

  EXPECT_TRUE(result.settled);
  // Frames 3,4,5 are the constant 0; the first is the baseline and the next
  // two are the repeats, so kSettledFrames=3 is reached on frame 6.
  EXPECT_EQ(result.frames, 6);
  EXPECT_EQ(pumps, 5) << "the loop must not pump after the frame it returns";
}

TEST(SettleLoopTest, ReportsFailureWhenTheFrameNeverSettles) {
  FrameSource frames{1000};

  const SettleResult result =
      RunSettleLoop(std::ref(frames), [] {}, /*max_frames=*/9);

  EXPECT_FALSE(result.settled);
  EXPECT_EQ(result.frames, 9);
  EXPECT_EQ(frames.grabs(), 9);
}

// The regression this loop exists for (task 468).
//
// The previous implementation bounded the wait with a 10 s QElapsedTimer while
// spending ~120 ms of event loop per frame. On a loaded machine each pass costs
// far more than that nominal interval, so the deadline expired after a handful
// of comparisons instead of the ~80 it buys when idle, and the capture failed
// claiming the widget was animating when the machine was simply busy.
//
// The sleep below is the one place in this file where wall time is the subject
// rather than something to wait out: it stands in for that load. The budget
// must be identical with and without it.
TEST(SettleLoopTest, ASlowPumpCostsWallTimeButNotComparisons) {
  constexpr int kBudget = 8;

  FrameSource fast_frames{1000};
  const SettleResult fast =
      RunSettleLoop(std::ref(fast_frames), [] {}, kBudget);

  FrameSource slow_frames{1000};
  const SettleResult slow = RunSettleLoop(
      std::ref(slow_frames),
      [] { std::this_thread::sleep_for(std::chrono::milliseconds{5}); },
      kBudget);

  EXPECT_EQ(slow.frames, fast.frames)
      << "a slow pump cost the loop comparisons, which is the wall-clock "
         "deadline defect this replaced";
  EXPECT_EQ(slow.frames, kBudget);
  EXPECT_FALSE(slow.settled);
}

// A widget that settles must settle identically however slow the machine is.
TEST(SettleLoopTest, ASlowPumpDoesNotChangeWhenAWidgetSettles) {
  FrameSource fast_frames{4};
  const SettleResult fast = RunSettleLoop(std::ref(fast_frames), [] {});

  FrameSource slow_frames{4};
  const SettleResult slow = RunSettleLoop(std::ref(slow_frames), [] {
    std::this_thread::sleep_for(std::chrono::milliseconds{5});
  });

  EXPECT_TRUE(fast.settled);
  EXPECT_TRUE(slow.settled);
  EXPECT_EQ(slow.frames, fast.frames);
}

TEST(SettleLoopTest, DefaultBudgetIsTheOneTheCaptureLoopDocuments) {
  FrameSource frames{1000};

  const SettleResult result = RunSettleLoop(std::ref(frames), [] {});

  EXPECT_FALSE(result.settled);
  EXPECT_EQ(result.frames, kMaxFrames);
}

}  // namespace scada::screenshot_generator
