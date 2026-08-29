#pragma once

#include <functional>
#include <string>
#include <string_view>

// The number of failed assertions recorded in the running test so far, or 0
// when called outside a test.
//
// Counted from the test's own `TestResult` rather than read off
// `Test::HasFailure()`, which is a bool and so cannot express "no *new*
// failure since this capture began".
int RecordedFailureCount();

// Refuses to publish a capture whose content assertions have failed.
//
// Every content check in this generator is `EXPECT_*` on purpose, so one bad
// render reports all of its problems at once instead of stopping at the first.
// `EXPECT` does not stop the test, though, so a save placed after the checks
// runs anyway -- and a regeneration then overwrites the tracked gallery file
// with the very render the assertions had just rejected. Red test, bad file,
// and the file is what gets committed.
//
// An empty surface is the case that makes this matter: it lays out perfectly,
// so nothing about the image looks wrong, and no dimension or layout check can
// distinguish it from a right one. The web generator's `capture.mjs` says the
// same thing about its own empty states.
//
// The scope is one capture, not one test, because two of the captures are
// sweeps: `CaptureAllWindows` and `CaptureDialogs` render dozens of images
// inside a single TEST_F, and `::testing::Test::HasFailure()` cannot tell this
// capture's failure from the previous one's. With a test-wide guard, one bad
// spec would suppress every image after it and turn a single content failure
// into a run-wide "not produced" cascade in check_screenshots.py.
class CapturePublishGuard {
 public:
  // Reads the number of failed assertions recorded so far. A parameter only so
  // that the guard's own tests can drive it: gtest records a failure by
  // failing the test, so a test exercising the live counter could not also
  // pass. `std::function` rather than a function pointer so a test can use a
  // capturing lambda instead of the file-scope counter a plain pointer would
  // force on it.
  using FailureCounter = std::function<int()>;

  // Snapshots the failure count. Construct it *before* the capture's content
  // assertions run -- a guard built afterwards sees their failures as
  // pre-existing and publishes anyway, which is the bug this class exists to
  // prevent.
  explicit CapturePublishGuard(
      std::string_view filename,
      FailureCounter count_failures = &RecordedFailureCount);

  CapturePublishGuard(const CapturePublishGuard&) = delete;
  CapturePublishGuard& operator=(const CapturePublishGuard&) = delete;

  // True when no assertion has failed since construction.
  //
  // On false it explains, on stderr, which image was not written and why. It
  // deliberately does not add a failure of its own: the assertion that makes
  // this false has already been reported, and a second one would describe the
  // consequence as though it were a separate defect.
  bool ShouldPublish() const;

 private:
  const std::string filename_;
  const FailureCounter count_failures_;
  const int failures_before_;
};
