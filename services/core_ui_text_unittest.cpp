#include "services/core_ui_text.h"

#include "scada/qualifier.h"
#include "scada/variant.h"

#include <gtest/gtest.h>

namespace {

class CoreUiTextTest : public ::testing::Test {
 protected:
  void TearDown() override {
    scada::SetStatusTextProvider(nullptr);
    scada::SetQualifierFlagTextProvider(nullptr);
    scada::SetBooleanTextProvider(nullptr);
    SetFallbackLabelProvider(nullptr);
  }
};

// The client tables are the only place these values are worded, so a code
// added to `scada::StatusCode` without a row here renders as the bare "Error"
// fallback. Mirrors core's `StatusTest.EveryBadCodeHasAnEntry`, which guards
// the symbolic names; this guards the sentences.
TEST_F(CoreUiTextTest, EveryBadCodeHasASentence) {
  const std::u16string fallback = StatusText(static_cast<scada::StatusCode>(
      (static_cast<unsigned>(scada::StatusSeverity::Bad) << 14) | 0x3FFF));
  constexpr auto kLast = scada::StatusCode::Bad_NotReadable;
  const unsigned bad = static_cast<unsigned>(scada::StatusCode::Bad);
  for (unsigned code = bad + 1; code <= static_cast<unsigned>(kLast); ++code) {
    EXPECT_NE(fallback, StatusText(static_cast<scada::StatusCode>(code)))
        << "Bad | " << (code - bad);
  }
}

// Every flag core renders in `ToString16(Qualifier)` needs a word here; a
// missing one renders as a bare space in the quality strip.
TEST_F(CoreUiTextTest, EveryRenderedQualifierFlagHasAWord) {
  constexpr unsigned kFlags[] = {
      scada::Qualifier::BAD,           scada::Qualifier::BACKUP,
      scada::Qualifier::OFFLINE,       scada::Qualifier::MANUAL,
      scada::Qualifier::MISCONFIGURED, scada::Qualifier::SIMULATED,
      scada::Qualifier::SPORADIC,      scada::Qualifier::STALE,
      scada::Qualifier::FAILED,
  };
  for (const unsigned flag : kFlags)
    EXPECT_FALSE(QualifierFlagText(flag).empty())
        << "flag 0x" << std::hex << flag;
}

TEST_F(CoreUiTextTest, InstalledTablesRenderThroughCore) {
  InstallCoreUiText();

  EXPECT_EQ(ToString16(scada::StatusCode::Bad_Timeout),
            StatusText(scada::StatusCode::Bad_Timeout));
  EXPECT_NE(ToString16(scada::StatusCode::Bad_Timeout), u"Bad_Timeout");

  EXPECT_EQ(ToString16(scada::Qualifier{scada::Qualifier::STALE}),
            QualifierFlagText(scada::Qualifier::STALE) + u" ");

  EXPECT_EQ(scada::Variant::TrueLabel(), BooleanText(true));
  EXPECT_EQ(scada::Variant::FalseLabel(), BooleanText(false));
  EXPECT_NE(scada::Variant::TrueLabel(), u"true");

  EXPECT_EQ(DefaultCloseLabel(),
            FallbackLabelText(FallbackLabel::kDefaultClose));
  EXPECT_EQ(UnknownDisplayName(),
            FallbackLabelText(FallbackLabel::kUnknownDisplayName));
  EXPECT_NE(DefaultCloseLabel(), u"1");
}

}  // namespace
