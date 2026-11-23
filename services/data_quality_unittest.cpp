#include "services/data_quality.h"

#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "scada/data_value.h"
#include "scada/qualifier.h"

#include <gtest/gtest.h>

#include <optional>

namespace {

using scada::aui::Quality;

TEST(DataQualityTest, DefaultQualifierIsGood) {
  EXPECT_EQ(DataQualityFor(scada::Qualifier{}), Quality::kGood);
}

TEST(DataQualityTest, GoodCaveatFlagsStayGood) {
  // Manual, backup, simulated, sporadic all carry a usable value — the
  // codebase's own StatusCode mapping (Qualifier::ToStatus) treats them as
  // Good_* — so they must not be demoted to uncertain/bad.
  EXPECT_EQ(DataQualityFor(scada::Qualifier{}.set_manual(true)),
            Quality::kGood);
  EXPECT_EQ(DataQualityFor(scada::Qualifier{}.set_backup(true)),
            Quality::kGood);
  EXPECT_EQ(DataQualityFor(scada::Qualifier{}.set_simulated(true)),
            Quality::kGood);
  EXPECT_EQ(DataQualityFor(scada::Qualifier{}.set_sporadic(true)),
            Quality::kGood);
}

TEST(DataQualityTest, StaleIsUncertain) {
  EXPECT_EQ(DataQualityFor(scada::Qualifier{}.set_stale(true)),
            Quality::kUncertain);
}

TEST(DataQualityTest, HardFailuresAreBad) {
  EXPECT_EQ(DataQualityFor(scada::Qualifier{}.set_bad(true)), Quality::kBad);
  EXPECT_EQ(DataQualityFor(scada::Qualifier{}.set_failed(true)), Quality::kBad);
  EXPECT_EQ(DataQualityFor(scada::Qualifier{}.set_online(false)),
            Quality::kBad);  // offline
  EXPECT_EQ(DataQualityFor(scada::Qualifier{}.set_misconfigured(true)),
            Quality::kBad);
}

TEST(DataQualityTest, BadDominatesUncertain) {
  // A point that is both stale and offline reads as bad — the worse bucket
  // wins so the operator is not lulled by the milder cue.
  scada::Qualifier qualifier;
  qualifier.set_stale(true).set_online(false);
  EXPECT_EQ(DataQualityFor(qualifier), Quality::kBad);
}

// Regression: a row bound to a node id the server does not have (the
// subscription is rejected Bad_WrongNodeId) never receives a value, so its
// DataValue keeps a default-constructed Qualifier — which is zero, and zero is
// not BAD. Mapping that through the Qualifier alone reported Good, so the grid
// showed a green "Достоверно" beside a permanently empty Value cell. Absent
// data must report no quality at all.
TEST(DataQualityTest, NeverDeliveredValueHasNoQuality) {
  EXPECT_FALSE(DataQualityFor(scada::DataValue{}).has_value());
}

TEST(DataQualityTest, DeliveredValueKeepsItsQualifierBand) {
  const scada::DataValue good{42.0, scada::Qualifier{}, scada::kNullTime,
                              scada::kNullTime};
  EXPECT_EQ(DataQualityFor(good), Quality::kGood);

  const scada::DataValue bad{42.0, scada::Qualifier{}.set_bad(true),
                             scada::kNullTime, scada::kNullTime};
  EXPECT_EQ(DataQualityFor(bad), Quality::kBad);
}

// A bad-quality value that carries no number still counts as delivered: the
// qualifier is non-zero, so the row reports Bad rather than "no data".
TEST(DataQualityTest, ValuelessButFlaggedReadingIsDelivered) {
  const scada::DataValue offline{scada::Variant{},
                                 scada::Qualifier{}.set_online(false),
                                 scada::kNullTime, scada::kNullTime};
  EXPECT_EQ(DataQualityFor(offline), Quality::kBad);
}

// Limit bits are not quality: an out-of-limit reading is still good data.
TEST(DataQualityTest, LimitBitsAreNotQuality) {
  scada::Qualifier qualifier;
  qualifier.set_limit(scada::Qualifier::LIMIT_HIHI);
  EXPECT_EQ(DataQualityFor(qualifier), Quality::kGood);
}

// Every surface names the band with these words, so a stale value reads the
// same in the Inspector's pill as in the table's Quality column (backlog 1120:
// the Inspector used to call STALE «Недостоверно»).
TEST(DataQualityTest, LabelsNameTheBand) {
  EXPECT_EQ(DataQualityLabel(Quality::kGood), Translate("Good"));
  EXPECT_EQ(DataQualityLabel(Quality::kUncertain), Translate("Uncertain"));
  EXPECT_EQ(DataQualityLabel(Quality::kBad), Translate("Bad"));
  EXPECT_EQ(DataQualityLabel(std::optional<Quality>{}), Translate("No data"));
  EXPECT_EQ(
      DataQualityLabel(DataQualityFor(scada::Qualifier{}.set_stale(true))),
      Translate("Uncertain"));
}

}  // namespace
