#include "modules/table/quality_mark.h"

#include "aui/severity_colors.h"
#include "scada/qualifier.h"

#include <gtest/gtest.h>

namespace {

using scada::aui::Quality;

TEST(QualityMarkTest, DefaultQualifierIsGood) {
  EXPECT_EQ(QualityFromQualifier(scada::Qualifier{}), Quality::kGood);
}

TEST(QualityMarkTest, GoodCaveatFlagsStayGood) {
  // Manual, backup, simulated, sporadic all carry a usable value — the
  // codebase's own StatusCode mapping (Qualifier::ToStatus) treats them as
  // Good_* — so they must not be demoted to uncertain/bad.
  EXPECT_EQ(QualityFromQualifier(scada::Qualifier{}.set_manual(true)),
            Quality::kGood);
  EXPECT_EQ(QualityFromQualifier(scada::Qualifier{}.set_backup(true)),
            Quality::kGood);
  EXPECT_EQ(QualityFromQualifier(scada::Qualifier{}.set_simulated(true)),
            Quality::kGood);
  EXPECT_EQ(QualityFromQualifier(scada::Qualifier{}.set_sporadic(true)),
            Quality::kGood);
}

TEST(QualityMarkTest, StaleIsUncertain) {
  EXPECT_EQ(QualityFromQualifier(scada::Qualifier{}.set_stale(true)),
            Quality::kUncertain);
}

TEST(QualityMarkTest, HardFailuresAreBad) {
  EXPECT_EQ(QualityFromQualifier(scada::Qualifier{}.set_bad(true)),
            Quality::kBad);
  EXPECT_EQ(QualityFromQualifier(scada::Qualifier{}.set_failed(true)),
            Quality::kBad);
  EXPECT_EQ(QualityFromQualifier(scada::Qualifier{}.set_online(false)),
            Quality::kBad);  // offline
  EXPECT_EQ(QualityFromQualifier(scada::Qualifier{}.set_misconfigured(true)),
            Quality::kBad);
}

TEST(QualityMarkTest, BadDominatesUncertain) {
  // A point that is both stale and offline reads as bad — the worse bucket
  // wins so the operator is not lulled by the milder cue.
  scada::Qualifier qualifier;
  qualifier.set_stale(true).set_online(false);
  EXPECT_EQ(QualityFromQualifier(qualifier), Quality::kBad);
}

}  // namespace
