#include "modules/about/about_info.h"

#include <gtest/gtest.h>

namespace {

AboutInfo FullInfo() {
  return AboutInfo{.product = u"Telecontrol SCADA",
                   .version = u"2.6.0",
                   .build_commit = u"9f3c21a",
                   .build_date = u"2026-09-18T10:00:00+03:00",
                   .runtime = u"Qt 6.8.2 · macOS 15.6 · arm64",
                   .server = u"opc.tcp://sub-north.tc.local:4840",
                   .user = u"operator1",
                   .has_session = true,
                   .connected = true};
}

TEST(AboutInfoTest, ProducesOneFixedBlock) {
  EXPECT_EQ(FormatAboutDetails(FullInfo(), u"2026-09-20T14:08:31+03:00"),
            std::u16string{
                u"Telecontrol SCADA — desktop client\n"
                u"Version    2.6.0\n"
                u"Build      9f3c21a · 2026-09-18T10:00:00+03:00\n"
                u"Runtime    Qt 6.8.2 · macOS 15.6 · arm64\n"
                u"Server     opc.tcp://sub-north.tc.local:4840\n"
                u"Connection connected\n"
                u"Signed in  operator1\n"
                u"Reported   2026-09-20T14:08:31+03:00\n"});
}

// The rule the shared screen sets, and the reason the payload is data rather
// than a transcript of the widgets: a row nobody can fill is absent, so a
// report taken before sign-in cannot be misread as one taken against a server.
TEST(AboutInfoTest, DropsEveryRowItCannotFill) {
  AboutInfo info = FullInfo();
  info.has_session = false;

  const std::u16string payload = FormatAboutDetails(info, u"");
  EXPECT_EQ(payload.find(u"Server"), std::u16string::npos);
  EXPECT_EQ(payload.find(u"Connection"), std::u16string::npos);
  EXPECT_EQ(payload.find(u"Signed in"), std::u16string::npos);
  EXPECT_EQ(payload.find(u"Reported"), std::u16string::npos);
  // What a report needs before sign-in survives.
  EXPECT_NE(payload.find(u"Build      9f3c21a"), std::u16string::npos);
  EXPECT_NE(payload.find(u"Runtime    Qt 6.8.2"), std::u16string::npos);
}

// The single exception to the rule above. Silence here would read as "this
// build has no identity"; the phrase says which of the two it is.
TEST(AboutInfoTest, SaysAnUnstampedBuildIsNotRecorded) {
  AboutInfo info = FullInfo();
  info.build_commit.clear();
  info.build_date.clear();

  EXPECT_NE(FormatAboutDetails(info, u"").find(u"Build      not recorded"),
            std::u16string::npos);
}

// A date with no commit behind it identifies nothing while reading exactly
// like one that does, so the commit alone is the honest form.
TEST(AboutInfoTest, FallsBackToTheBareCommitWithoutADate) {
  AboutInfo info = FullInfo();
  info.build_date.clear();

  EXPECT_NE(FormatAboutDetails(info, u"").find(u"Build      9f3c21a\n"),
            std::u16string::npos);
}

// An anonymous session is a session: the row says so rather than vanishing,
// because "nobody was signed in" and "the dialog could not tell" are different
// answers to the same question in a fault report.
TEST(AboutInfoTest, NamesAnAnonymousSessionRatherThanDroppingTheRow) {
  AboutInfo info = FullInfo();
  info.user.clear();

  EXPECT_NE(FormatAboutDetails(info, u"").find(u"Signed in  anonymous"),
            std::u16string::npos);
}

// The words the payload uses are produced HERE, from the flag, rather than
// carried in the struct — which is what keeps them from drifting away from the
// words the dialog shows for the same state.
TEST(AboutInfoTest, ReportsALostLinkAgainstTheServerItStillNames) {
  AboutInfo info = FullInfo();
  info.connected = false;

  const std::u16string payload = FormatAboutDetails(info, u"");
  EXPECT_NE(payload.find(u"Connection not connected"), std::u16string::npos);
  EXPECT_NE(payload.find(u"Server     opc.tcp://sub-north.tc.local:4840"),
            std::u16string::npos);
}

// The label column is shared with the web client's payload on purpose: two
// reports of one fault are routinely read side by side, and a column that
// drifts between the realms is the kind of difference a reader stops to think
// about. `Connection` is exactly the column width, which is what fixes it.
TEST(AboutInfoTest, AlignsEveryValueInOneColumn) {
  const std::u16string payload =
      FormatAboutDetails(FullInfo(), u"2026-09-20T14:08:31+03:00");

  size_t line_start = payload.find(u'\n') + 1;
  size_t checked = 0;
  while (line_start < payload.size()) {
    const size_t line_end = payload.find(u'\n', line_start);
    ASSERT_NE(line_end, std::u16string::npos);
    const std::u16string line = payload.substr(line_start, line_end - line_start);
    // Column 11 (0-based) is where every value begins.
    EXPECT_NE(line.size(), 11u) << "a row with no value reached the payload";
    EXPECT_NE(line[11], u' ') << "value not in the shared column";
    ++checked;
    line_start = line_end + 1;
  }
  EXPECT_EQ(checked, 7u);
}

}  // namespace
