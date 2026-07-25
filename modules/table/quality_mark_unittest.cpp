#include "modules/table/quality_mark.h"

#include "aui/severity_colors.h"
#include "scada/data_value.h"
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

// Regression: a row bound to a node id the server does not have (the
// subscription is rejected Bad_WrongNodeId) never receives a value, so its
// DataValue keeps a default-constructed Qualifier — which is zero, and zero is
// not BAD. Mapping that through the Qualifier alone reported Good, so the grid
// showed a green "Достоверно" beside a permanently empty Value cell. Absent
// data must report no quality at all.
TEST(QualityMarkTest, NeverDeliveredValueHasNoQuality) {
  EXPECT_FALSE(QualityFromValue(scada::DataValue{}).has_value());
}

TEST(QualityMarkTest, DeliveredValueKeepsItsQualifierBand) {
  const scada::DataValue good{42.0, scada::Qualifier{}, scada::kNullTime,
                              scada::kNullTime};
  EXPECT_EQ(QualityFromValue(good), Quality::kGood);

  const scada::DataValue bad{42.0, scada::Qualifier{}.set_bad(true),
                             scada::kNullTime, scada::kNullTime};
  EXPECT_EQ(QualityFromValue(bad), Quality::kBad);
}

// A bad-quality value that carries no number still counts as delivered: the
// qualifier is non-zero, so the row reports Bad rather than "no data".
TEST(QualityMarkTest, ValuelessButFlaggedReadingIsDelivered) {
  const scada::DataValue offline{scada::Variant{},
                                 scada::Qualifier{}.set_online(false),
                                 scada::kNullTime, scada::kNullTime};
  EXPECT_EQ(QualityFromValue(offline), Quality::kBad);
}

}  // namespace
