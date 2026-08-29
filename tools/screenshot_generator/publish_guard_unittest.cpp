#include "publish_guard.h"

#include <gtest/gtest.h>

namespace {

// A counter the test drives directly. The live one cannot be used here: gtest
// records a failure by failing the test, so a test that made the real counter
// move could not also pass.
class FakeFailures {
 public:
  CapturePublishGuard::FailureCounter Counter() {
    return [this] { return count_; };
  }
  void Fail() { ++count_; }
  void SetTo(int count) { count_ = count; }

 private:
  int count_ = 0;
};

TEST(CapturePublishGuardTest, PublishesWhenNothingFailed) {
  FakeFailures failures;
  CapturePublishGuard guard("quiet.png", failures.Counter());
  EXPECT_TRUE(guard.ShouldPublish());
}

// The regression this class exists for: the capture's own content assertions
// failed, so the image it would write documents a surface the checks have just
// rejected. Before the guard, the save ran regardless.
TEST(CapturePublishGuardTest, RefusesAfterItsOwnCaptureFailed) {
  FakeFailures failures;
  CapturePublishGuard guard("broken.png", failures.Counter());
  failures.Fail();
  EXPECT_FALSE(guard.ShouldPublish());
}

// A sweep renders dozens of images inside one TEST_F, so a capture must be
// judged on failures raised since *it* began. Test-wide state -- which is all
// Test::HasFailure() offers -- would suppress every image after the first bad
// one and turn one content failure into a run-wide "not produced" cascade.
TEST(CapturePublishGuardTest, EarlierCapturesFailureDoesNotSuppressALaterOne) {
  FakeFailures failures;
  failures.SetTo(3);
  CapturePublishGuard guard("later.png", failures.Counter());
  EXPECT_TRUE(guard.ShouldPublish());
}

// The verdict tracks the counter rather than latching at the first read: the
// dialog sweep asks once per dialog, and a guard that answered from a cached
// snapshot would publish an image whose checks failed after the first ask.
TEST(CapturePublishGuardTest, VerdictFollowsFailuresRaisedAfterTheFirstRead) {
  FakeFailures failures;
  CapturePublishGuard guard("stable.png", failures.Counter());
  EXPECT_TRUE(guard.ShouldPublish());
  failures.Fail();
  EXPECT_FALSE(guard.ShouldPublish());
  EXPECT_FALSE(guard.ShouldPublish());
}

// The live counter reads the running test's own TestResult. In a test that has
// failed nothing it must read zero -- if it read something else, every guard in
// the generator would be comparing against a moving baseline.
TEST(CapturePublishGuardTest, LiveCounterIsZeroInAPassingTest) {
  EXPECT_EQ(RecordedFailureCount(), 0);
}

}  // namespace
