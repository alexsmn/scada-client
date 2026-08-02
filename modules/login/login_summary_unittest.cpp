#include "modules/login/login_summary.h"

#include <gtest/gtest.h>

namespace {

TEST(LoginConnectionSummaryTest, JoinsBackendAndServer) {
  EXPECT_EQ(LoginConnectionSummary(u"Телеконтроль", u"localhost"),
            u"Телеконтроль · localhost");
}

// Either half alone is shown without a dangling separator.
TEST(LoginConnectionSummaryTest, ShowsWhicheverHalfIsKnown) {
  EXPECT_EQ(LoginConnectionSummary(u"OPC UA", u""), u"OPC UA");
  EXPECT_EQ(LoginConnectionSummary(u"", u"opc.tcp://localhost:4840"),
            u"opc.tcp://localhost:4840");
}

// Nothing known renders nothing, so the caller hides the line entirely.
TEST(LoginConnectionSummaryTest, EmptyWhenNothingIsKnown) {
  EXPECT_TRUE(LoginConnectionSummary(u"", u"").empty());
}

// A blank-but-not-empty entry (a stray space in a saved connection) counts as
// unknown rather than producing " · localhost".
TEST(LoginConnectionSummaryTest, BlankEntriesCountAsUnknown) {
  EXPECT_EQ(LoginConnectionSummary(u"  ", u"localhost"), u"localhost");
  EXPECT_TRUE(LoginConnectionSummary(u" ", u"\t").empty());
}

// Surrounding whitespace never reaches the rendered line.
TEST(LoginConnectionSummaryTest, TrimsSurroundingWhitespace) {
  EXPECT_EQ(LoginConnectionSummary(u" OPC UA ", u" localhost "),
            u"OPC UA · localhost");
}

}  // namespace
