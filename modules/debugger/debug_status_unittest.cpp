#include "modules/debugger/debug_status.h"

#include <gtest/gtest.h>

namespace {

using RequestPhase = scada::SessionDebugger::RequestPhase;

RequestTableModel::Request MakeRequest(RequestTableModel::RequestId id,
                                       RequestPhase phase,
                                       std::string title) {
  return RequestTableModel::Request{
      .request_id = id, .phase = phase, .title = std::move(title)};
}

TEST(DebugStatusTest, MapsPhaseToBand) {
  EXPECT_EQ(DebugStatusFor(RequestPhase::Running), DebugStatus::kRunning);
  EXPECT_EQ(DebugStatusFor(RequestPhase::Succeeded), DebugStatus::kOk);
  EXPECT_EQ(DebugStatusFor(RequestPhase::Failed), DebugStatus::kError);
}

TEST(DebugRequestMatchesTest, EmptyQueryMatchesEverything) {
  const auto request = MakeRequest(1, RequestPhase::Succeeded, "Read");
  EXPECT_TRUE(DebugRequestMatches(request, u""));
}

TEST(DebugRequestMatchesTest, MatchesTitleCaseInsensitively) {
  const auto request = MakeRequest(1, RequestPhase::Succeeded, "BrowseNext");
  EXPECT_TRUE(DebugRequestMatches(request, u"browse"));
  EXPECT_TRUE(DebugRequestMatches(request, u"NEXT"));
  EXPECT_FALSE(DebugRequestMatches(request, u"write"));
}

TEST(DebugRequestMatchesTest, MatchesRequestIdSubstring) {
  const auto request = MakeRequest(4021, RequestPhase::Running, "Read");
  EXPECT_TRUE(DebugRequestMatches(request, u"402"));
  EXPECT_TRUE(DebugRequestMatches(request, u"4021"));
  EXPECT_FALSE(DebugRequestMatches(request, u"999"));
}

}  // namespace
