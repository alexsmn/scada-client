#include "events/event_statistics.h"

#include "aui/translation.h"
#include "scada/node_id.h"

#include <gtest/gtest.h>

namespace events {
namespace {

using scada::aui::SeverityLevel;

const scada::NodeId kPump{1, 3};
const scada::NodeId kValve{2, 3};

// Every occurrence is counted once in the total, once in its band and once
// against its object; the unacknowledged ones again beside each count.
TEST(EventStatisticsTest, CountsByBandAndByObject) {
  const EventStatisticsSample samples[] = {
      {.object = kPump,
       .object_name = u"Насос",
       .severity = 900,
       .pending = true},
      {.object = kPump, .object_name = u"Насос", .severity = 900},
      {.object = kPump, .object_name = u"Насос", .severity = 1},
      {.object = kValve,
       .object_name = u"Задвижка",
       .severity = 600,
       .pending = true},
  };

  const EventStatistics statistics = ComputeEventStatistics(samples);

  EXPECT_EQ(statistics.total, (EventStatistics::Tally{4, 2}));
  EXPECT_EQ(statistics.band(SeverityLevel::kCritical),
            (EventStatistics::Tally{2, 1}));
  EXPECT_EQ(statistics.band(SeverityLevel::kWarning),
            (EventStatistics::Tally{1, 1}));
  EXPECT_EQ(statistics.band(SeverityLevel::kNone),
            (EventStatistics::Tally{1, 0}));

  ASSERT_EQ(statistics.by_object.size(), 2u);
  EXPECT_EQ(statistics.by_object[0].name, u"Насос");
  EXPECT_EQ(statistics.by_object[0].tally, (EventStatistics::Tally{3, 1}));
  EXPECT_EQ(statistics.by_object[0].max_severity, 900u);
  EXPECT_EQ(statistics.by_object[1].name, u"Задвижка");
  EXPECT_EQ(statistics.by_object[1].tally, (EventStatistics::Tally{1, 1}));
}

// Objects with as many events read in alphabetical order; two objects with
// one name stay two rows, since they are two objects.
TEST(EventStatisticsTest, OrdersEqualCountsByNameAndKeepsNamesakesApart) {
  const EventStatisticsSample samples[] = {
      {.object = kPump, .object_name = u"Яма"},
      {.object = kValve, .object_name = u"Ёмкость"},
      {.object = scada::NodeId{3, 3}, .object_name = u"Ёмкость"},
  };

  const EventStatistics statistics = ComputeEventStatistics(samples);

  ASSERT_EQ(statistics.by_object.size(), 3u);
  EXPECT_EQ(statistics.by_object[0].name, u"Ёмкость");
  EXPECT_EQ(statistics.by_object[1].name, u"Ёмкость");
  EXPECT_EQ(statistics.by_object[2].name, u"Яма");
}

TEST(EventStatisticsTest, NothingShownCountsNothing) {
  const EventStatistics statistics = ComputeEventStatistics({});
  EXPECT_EQ(statistics.total, EventStatistics::Tally{});
  EXPECT_TRUE(statistics.by_object.empty());
}

// The copied text pastes into a spreadsheet one value per cell: tab-separated,
// a heading row per table, the period first.
TEST(EventStatisticsTest, TheTextIsTabSeparated) {
  const EventStatisticsSample samples[] = {
      {.object = kPump,
       .object_name = u"Насос",
       .severity = 900,
       .pending = true},
  };
  const std::u16string text =
      FormatEventStatisticsText(ComputeEventStatistics(samples), u"Сутки");

  EXPECT_TRUE(text.starts_with(u"Сутки\n\n"));
  EXPECT_NE(text.find(Translate("Severity") + u"\t" + Translate("Count")),
            std::u16string::npos);
  EXPECT_NE(text.find(Translate("Total") + u"\t1\t1\n"), std::u16string::npos);
  EXPECT_NE(text.find(u"Насос\t1\t1\n"), std::u16string::npos);
}

}  // namespace
}  // namespace events
