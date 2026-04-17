#include "components/summary/summary_model.h"

#include "base/test/test_time.h"
#include "common/aggregation.h"
#include "resources/common_resources.h"
#include "components/summary/summary_component.h"
#include "controller/window_info.h"
#include "node_service/node_service_mock.h"
#include "profile/window_definition.h"
#include "timed_data/timed_data_fake.h"
#include "timed_data/timed_data_service_fake.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

WindowItem MakeItem(std::string_view path) {
  return WindowItem{"Item"}.SetString("path", path).SetInt("width", 123);
}

}  // namespace

class SummaryModelTest : public Test {
 protected:
  aui::GridCell GetCellAt(int column_index, base::Time time);

  // The node service is only used when adding contained items.
  StrictMock<MockNodeService> node_service_;

  FakeTimedDataService timed_data_service_;

  SummaryModel summary_model_{
      SummaryModelContext{node_service_, timed_data_service_}};
};

aui::GridCell SummaryModelTest::GetCellAt(int column_index, base::Time time) {
  int row_index = summary_model_.GetRowForTime(time);

  aui::GridCell cell{.row = row_index, .column = column_index};
  summary_model_.GetCell(cell);

  return cell;
}

TEST_F(SummaryModelTest, AddColumn) {
  timed_data_service_.AddTimedData("item1");
  timed_data_service_.AddTimedData("item2");

  summary_model_.Load(
      WindowDefinition{kSummaryWindowInfo}.AddItem(MakeItem("item1")));

  // ACT
  int column_index = summary_model_.AddColumn("item2");

  ASSERT_EQ(column_index, 1);
  EXPECT_EQ(summary_model_.column_model().GetCount(), 2);
  EXPECT_EQ(summary_model_.column_model().GetTitle(1), u"item2");
}

TEST_F(SummaryModelTest, HourlySummaryForDayShows24Rows) {
  auto start_time = TestTimeFromString("15 Nov 2004 10:00:00 UTC");
  auto time_range =
      TimeRange{start_time, start_time + base::TimeDelta::FromHours(24)};

  auto timed_data = timed_data_service_.AddTimedData("item1");

  for (int i = 0; i < 24; ++i) {
    auto timestamp = start_time + base::TimeDelta::FromHours(i);
    timed_data->data_values.emplace_back(/*value=*/i,
                                         /*qualifier=*/scada::Qualifier{},
                                         /*source_timestamp=*/timestamp,
                                         /*server_timestamp=*/timestamp);
  }

  // ACT
  summary_model_.Load(WindowDefinition{kSummaryWindowInfo}
                          .AddItem(MakeItem("item1"))
                          .AddItem("TimeRange", time_range));

  EXPECT_EQ(summary_model_.GetTimeRange(), time_range);
  EXPECT_EQ(summary_model_.row_model().GetCount(), 24);
  EXPECT_EQ(summary_model_.GetRowTime(0), start_time);
  EXPECT_EQ(summary_model_.GetRowTime(23),
            start_time + base::TimeDelta::FromHours(23));
  EXPECT_EQ(summary_model_.GetCellText(/*row=*/0, /*column=*/0), u"0");
  EXPECT_EQ(summary_model_.GetCellText(/*row=*/23, /*column=*/0), u"23");
}

TEST_F(SummaryModelTest, Load) {
  timed_data_service_.AddTimedData("item1");
  timed_data_service_.AddTimedData("item2");

  auto time_range = TimeRange{TestTimeFromString("15 Nov 2004 12:45:26 UTC"),
                              TestTimeFromString("16 Nov 2004 12:45:26 UTC")};

  auto window_def =
      WindowDefinition{kSummaryWindowInfo}
          .AddItem(MakeItem("item1"))
          .AddItem(MakeItem("item2"))
          .AddItem("TimeRange", time_range)
          .AddItem("Interval", scada::Duration::FromMinutes(30))
          .AddItem("AggregateType", scada::id::AggregateFunction_Maximum);

  summary_model_.Load(window_def);

  EXPECT_EQ(scada::id::AggregateFunction_Maximum,
            summary_model_.aggregate_type());

  EXPECT_EQ(scada::Duration::FromMinutes(30), summary_model_.interval());
  EXPECT_EQ(time_range, summary_model_.time_range());

  ASSERT_EQ(summary_model_.row_model().GetCount(), 49);  // 30-min interval

  EXPECT_EQ(summary_model_.GetRowTime(0),
            TestTimeFromString("15 Nov 2004 12:30:00 UTC"));

  EXPECT_EQ(summary_model_.GetRowTime(48),
            TestTimeFromString("16 Nov 2004 12:30:00 UTC"));

  EXPECT_EQ(48, summary_model_.GetRowForTime(
                    TestTimeFromString("16 Nov 2004 12:56:43 UTC")));
}

TEST_F(SummaryModelTest, Save) {
  timed_data_service_.AddTimedData("item1");
  timed_data_service_.AddTimedData("item2");

  auto window_def =
      WindowDefinition{kSummaryWindowInfo}
          .AddItem(MakeItem("item1"))
          .AddItem(MakeItem("item2"))
          .AddItem("TimeRange",
                   TimeRange{TestTimeFromString("15 Nov 2004 12:45:26 UTC"),
                             TestTimeFromString("16 Nov 2004 12:45:26 UTC")})
          .AddItem("Interval", scada::Duration::FromMinutes(30))
          .AddItem("AggregateType", scada::id::AggregateFunction_Maximum);

  summary_model_.Load(window_def);

  WindowDefinition window_definition{kSummaryWindowInfo};
  summary_model_.Save(window_definition);
  EXPECT_EQ(window_def, window_definition);
}

TEST_F(SummaryModelTest, CellsAreGreyWhileLoading) {
  auto timed_data = timed_data_service_.AddTimedData("item1");

  timed_data->ready_ranges = {{TestTimeFromString("15 Nov 2004 14:00:00 UTC"),
                               TestTimeFromString("15 Nov 2004 15:00:00 UTC")}};

  summary_model_.Load(
      WindowDefinition{kSummaryWindowInfo}
          .AddItem(MakeItem("item1"))
          .AddItem("TimeRange",
                   TimeRange{TestTimeFromString("15 Nov 2004 12:45:26 UTC"),
                             TestTimeFromString("16 Nov 2004 12:45:26 UTC")})
          .AddItem("Interval", scada::Duration::FromMinutes(30))
          .AddItem("AggregateType", scada::id::AggregateFunction_Maximum));

  EXPECT_EQ(aui::ColorCode::DarkGray,
            GetCellAt(/*column_index=*/0,
                      TestTimeFromString("15 Nov 2004 13:30:00 UTC"))
                .cell_color);

  EXPECT_EQ(aui::ColorCode::White,
            GetCellAt(/*column_index=*/0,
                      TestTimeFromString("15 Nov 2004 14:00:00 UTC"))
                .cell_color);

  EXPECT_EQ(aui::ColorCode::White,
            GetCellAt(/*column_index=*/0,
                      TestTimeFromString("15 Nov 2004 14:30:00 UTC"))
                .cell_color);

  EXPECT_EQ(aui::ColorCode::DarkGray,
            GetCellAt(/*column_index=*/0,
                      TestTimeFromString("15 Nov 2004 15:00:00 UTC"))
                .cell_color);
}
