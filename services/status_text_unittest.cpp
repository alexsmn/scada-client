#include "services/status_text.h"

#include <gtest/gtest.h>

namespace {

// The client table is the only place status codes are worded, so a code added
// to `scada::StatusCode` without a row here renders as the bare "Error"
// fallback. Mirrors core's `StatusTest.EveryBadCodeHasAnEntry`, which guards
// the symbolic names; this guards the sentences.
TEST(StatusTextTest, EveryBadCodeHasASentence) {
  const std::u16string fallback = StatusText(static_cast<scada::StatusCode>(
      (static_cast<unsigned>(scada::StatusSeverity::Bad) << 14) | 0x3FFF));
  constexpr auto kLast = scada::StatusCode::Bad_NotReadable;
  const unsigned bad = static_cast<unsigned>(scada::StatusCode::Bad);
  for (unsigned code = bad + 1; code <= static_cast<unsigned>(kLast); ++code) {
    EXPECT_NE(fallback, StatusText(static_cast<scada::StatusCode>(code)))
        << "Bad | " << (code - bad);
  }
}

TEST(StatusTextTest, InstalledTableRendersThroughToString16) {
  InstallStatusText();
  EXPECT_EQ(ToString16(scada::StatusCode::Bad_Timeout),
            StatusText(scada::StatusCode::Bad_Timeout));
  EXPECT_NE(ToString16(scada::StatusCode::Bad_Timeout), u"Bad_Timeout");
  scada::SetStatusTextProvider(nullptr);
}

}  // namespace
