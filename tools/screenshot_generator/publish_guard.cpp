#include "publish_guard.h"

#include <gtest/gtest.h>

#include <iostream>
#include <utility>

int RecordedFailureCount() {
  const ::testing::TestInfo* info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  if (!info)
    return 0;
  const ::testing::TestResult* result = info->result();
  if (!result)
    return 0;

  // gtest records only failing parts for the ordinary EXPECT/ASSERT macros,
  // but SUCCEED() records a passing one, so count the failures rather than
  // taking total_part_count() as a failure count.
  int failures = 0;
  for (int i = 0; i < result->total_part_count(); ++i) {
    if (result->GetTestPartResult(i).failed())
      ++failures;
  }
  return failures;
}

CapturePublishGuard::CapturePublishGuard(std::string_view filename,
                                         FailureCounter count_failures)
    : filename_(filename),
      count_failures_(std::move(count_failures)),
      failures_before_(count_failures_()) {}

bool CapturePublishGuard::ShouldPublish() const {
  if (count_failures_() == failures_before_)
    return true;

  std::cerr << "refusing to write " << filename_
            << ": it failed the content checks reported above, and an image "
               "of it would document a surface the client does not ship"
            << std::endl;
  return false;
}
