#include "events/qt/event_filter_bar.h"

#include "aui/test/app_environment.h"
#include "base/relative_time_range.h"
#include "base/test/test_executor.h"
#include "base/time/time.h"
#include "node_service/static/static_node_service.h"
#include "scada/date_time.h"

#include <gtest/gtest.h>

#include <QLineEdit>

#include <memory>
#include <optional>
#include <string>

namespace {

// The bar offers a non-empty set of quick-pick period ranges.
TEST(EventFilterBarPeriodTest, OffersQuickPickRanges) {
  EXPECT_GE(EventPeriodRanges().size(), 2u);
}

// Every fixed preset range reflects onto its own index.
TEST(EventFilterBarPeriodTest, FixedRangeReflectsOntoItsPreset) {
  const auto& ranges = EventPeriodRanges();
  for (size_t i = 0; i < ranges.size(); ++i)
    EXPECT_EQ(EventPeriodPresetIndex(ranges[i]), static_cast<int>(i)) << i;
}

// The 15-minute and hourly interval presets are present and reflect correctly.
TEST(EventFilterBarPeriodTest, IntervalPresetsMatch) {
  EXPECT_EQ(EventPeriodPresetIndex(
                scada::RelativeTimeRange{std::chrono::minutes(15)}),
            0);
  EXPECT_EQ(
      EventPeriodPresetIndex(scada::RelativeTimeRange{std::chrono::hours(1)}),
      1);
}

// A range that matches no quick-pick (an arbitrary custom window, or an
// interval the bar does not offer) reflects as "no preset" (-1) so the selector
// does not misrepresent it.
TEST(EventFilterBarPeriodTest, UnmatchedRangeHasNoPreset) {
  scada::RelativeTimeRange custom{scada::Time{},
                                  scada::Time{} + std::chrono::hours(3)};
  EXPECT_EQ(EventPeriodPresetIndex(custom), -1);

  EXPECT_EQ(EventPeriodPresetIndex(
                scada::RelativeTimeRange{std::chrono::minutes(42)}),
            -1);
}

// The search box filters once typing pauses, so a keystroke alone applies
// nothing; Enter and clearing the box apply at once. The pause itself is a
// QTimer and is not waited out here.
class EventFilterBarSearchTest : public testing::Test {
 protected:
  QLineEdit* Search() {
    return bar_->findChild<QLineEdit*>(QStringLiteral("eventSearch"));
  }

  AppEnvironment app_env_;
  TestExecutor executor_;
  StaticNodeService node_service_;
  std::optional<std::u16string> applied_;

  std::unique_ptr<QWidget> bar_{MakeEventFilterBar(EventFilterBarContext{
      .executor = executor_,
      .node_service = node_service_,
      .text_filter = u"насос",
      .on_text_filter = [this](const std::u16string& text) {
        applied_ = text;
      }})};
};

TEST_F(EventFilterBarSearchTest, ShowsTheRestoredSearch) {
  ASSERT_NE(Search(), nullptr);
  EXPECT_EQ(Search()->text().toStdU16String(), u"насос");
  EXPECT_FALSE(applied_);
}

TEST_F(EventFilterBarSearchTest, TypingWaitsAndEnterApplies) {
  Search()->setText(QStringLiteral("авария"));
  EXPECT_FALSE(applied_);

  emit Search() -> returnPressed();
  EXPECT_EQ(applied_, u"авария");
}

TEST_F(EventFilterBarSearchTest, ClearingAppliesAtOnce) {
  Search()->clear();
  EXPECT_EQ(applied_, u"");
}

}  // namespace
