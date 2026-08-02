#include "base/blinker.h"

#include "base/test/scoped_mock_clock_override.h"

#include <gtest/gtest.h>

namespace {

// A half-period boundary, so a test can step within and across one phase
// without straddling a flip. The chrono epoch is exactly on a boundary
// (0 % anything == 0), which makes it the natural anchor.
constexpr scada::Time kPhaseStart = scada::Time{};

// Regression: blink state used to be a free-running toggle driven by a
// repeating timer, so it depended on when the manager happened to be
// constructed and on how long a run had been going. Screenshot captures
// inherited that: the same alarm row rendered highlighted in one run and plain
// in the next, which makes a capture diff — the project's visual-regression
// signal — useless.
TEST(BlinkerTest, PhaseIsStableWithinAHalfPeriod) {
  EXPECT_EQ(BlinkPhaseAt(kPhaseStart),
            BlinkPhaseAt(kPhaseStart + kBlinkHalfPeriod / 2));
  EXPECT_EQ(BlinkPhaseAt(kPhaseStart),
            BlinkPhaseAt(kPhaseStart + kBlinkHalfPeriod -
                         std::chrono::microseconds{1}));
}

TEST(BlinkerTest, PhaseFlipsEveryHalfPeriod) {
  EXPECT_NE(BlinkPhaseAt(kPhaseStart),
            BlinkPhaseAt(kPhaseStart + kBlinkHalfPeriod));
  EXPECT_EQ(BlinkPhaseAt(kPhaseStart),
            BlinkPhaseAt(kPhaseStart + 2 * kBlinkHalfPeriod));
  EXPECT_NE(BlinkPhaseAt(kPhaseStart),
            BlinkPhaseAt(kPhaseStart - kBlinkHalfPeriod));
}

// Instants before the chrono epoch alternate like any other. The project's null
// timestamp is the 1601 epoch, so negative counts are not hypothetical here.
TEST(BlinkerTest, PhaseFlipsEveryHalfPeriodBeforeTheEpoch) {
  const scada::Time before_epoch = kPhaseStart - std::chrono::hours{24};

  EXPECT_NE(BlinkPhaseAt(before_epoch),
            BlinkPhaseAt(before_epoch + kBlinkHalfPeriod));
  EXPECT_EQ(BlinkPhaseAt(before_epoch),
            BlinkPhaseAt(before_epoch + 2 * kBlinkHalfPeriod));
}

// The half-period on either side of the epoch must be two distinct phases.
// Truncating integer division rounds both toward quotient 0, which would merge
// them into one blink of double length.
TEST(BlinkerTest, PhaseFlipsAcrossTheEpoch) {
  EXPECT_NE(BlinkPhaseAt(kPhaseStart),
            BlinkPhaseAt(kPhaseStart - std::chrono::microseconds{1}));
  EXPECT_EQ(BlinkPhaseAt(kPhaseStart - std::chrono::microseconds{1}),
            BlinkPhaseAt(kPhaseStart - kBlinkHalfPeriod));
}

// What the screenshot generator relies on: with the clock frozen, every read of
// the blink state agrees, so a capture cannot catch a half-lit frame.
TEST(BlinkerTest, FrozenClockGivesOneStableState) {
  const scada::base::ScopedMockClockOverride clock;

  const bool state = BlinkPhaseAt(scada::Now());
  for (int i = 0; i < 10; ++i)
    EXPECT_EQ(BlinkPhaseAt(scada::Now()), state);
}

}  // namespace
