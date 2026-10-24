#include "print/service/print_header.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace {

constexpr scada::Time kPrintedAt =
    scada::Time{} + std::chrono::hours{24 * 365 * 54};

// A printout used to carry column titles and cells only, so a sheet of paper
// could not say what it was a record of (backlog 875).
TEST(PrintHeaderTest, CarriesTitlePeriodDescriptionAndPrintTime) {
  const scada::TimeRange period{kPrintedAt - std::chrono::hours{24},
                                kPrintedAt};

  const PrintHeader header =
      MakePrintHeader(u"Summary", period, u"Maximum, 1-Hour", kPrintedAt);

  EXPECT_EQ(header.title, u"Summary");
  ASSERT_THAT(header.lines, SizeIs(3));
  EXPECT_NE(header.lines[0].find(u"Period"), std::u16string::npos);
  EXPECT_EQ(header.lines[1], u"Maximum, 1-Hour");
  EXPECT_NE(header.lines[2].find(u"Printed"), std::u16string::npos);
}

TEST(PrintHeaderTest, AViewWithoutAPeriodPrintsNoPeriodLine) {
  const PrintHeader header =
      MakePrintHeader(u"Table", std::nullopt, {}, kPrintedAt);

  ASSERT_THAT(header.lines, SizeIs(1));
  EXPECT_NE(header.lines[0].find(u"Printed"), std::u16string::npos);
}

}  // namespace
