#include "components/summary/summary_model.h"

#include "base/test/test_time.h"
#include "common/aggregation.h"
#include "common_resources.h"
#include "components/summary/summary_component.h"
#include "controller/window_definition.h"
#include "node_service/node_service_mock.h"
#include "timed_data/timed_data_mock.h"
#include "timed_data/timed_data_service_mock.h"
#include "controller/window_info.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

WindowItem MakeItem(std::string_view path) {
  return WindowItem{"Item"}.SetString("path", path).SetInt("width", 123);
}

}  // namespace

class SummaryModelTest : public Test {
 public:
  SummaryModelTest();

 protected:
  struct TestColumn {
    int index = 0;
    std::shared_ptr<MockTimedData> timed_data;
  };

  TestColumn AddColumn();
  aui::GridCell GetCellAt(int column_index, base::Time time);

  // The item order must correspond to |SummaryModel::Save()| order.
  const WindowDefinition kWindowDefinition =
      WindowDefinition{kSummaryWindowInfo}
          .AddItem(MakeItem("item1-formula"))
          .AddItem(MakeItem("item2-formula"))
          .AddItem("TimeRange", kTimeRange)
          .AddItem("Interval", kInterval)
          .AddItem("AggregateType", kAggregateType);

  StrictMock<MockNodeService> node_service_;
  StrictMock<MockTimedDataService> timed_data_service_;

  SummaryModel summary_model_{
      SummaryModelContext{node_service_, timed_data_service_}};

  inline static const auto kTimeRange =
      TimeRange{TestTimeFromString("15 Nov 2004 12:45:26 UTC"),
                TestTimeFromString("16 Nov 2004 12:45:26 UTC")};
  inline static const auto kInterval = scada::Duration::FromMinutes(30);
  inline static const auto kAggregateType =
      scada::id::AggregateFunction_Maximum;
  inline static const std::string_view kFormula = "test-formula";
  inline static const scada::AggregateFilter kAggregateFilter{
      .start_time = scada::GetLocalAggregateStartTime(),
      .interval = kInterval,
      .aggregate_type = kAggregateType};
};

SummaryModelTest::SummaryModelTest() {
  EXPECT_CALL(timed_data_service_, GetFormulaTimedData(_, _))
      .Times(2)
      .WillRepeatedly([](std::string_view formula,
                         const scada::AggregateFilter& aggregation) {
        auto timed_data = std::make_shared<NiceMock<MockTimedData>>();
        ON_CALL(*timed_data, GetFormula(_))
            .WillByDefault(Return(std::string{formula}));
        return timed_data;
      });

  summary_model_.Load(kWindowDefinition);
}

SummaryModelTest::TestColumn SummaryModelTest::AddColumn() {
  auto timed_data = std::make_shared<NiceMock<MockTimedData>>();

  EXPECT_CALL(timed_data_service_,
              GetFormulaTimedData(kFormula, kAggregateFilter))
      .WillOnce(Return(timed_data));

  EXPECT_CALL(*timed_data, AddObserver(_));

  // Start time is rounded down, end time is rounded up.
  EXPECT_CALL(
      *timed_data,
      AddViewObserver(_, scada::DateTimeRange{
                             TestTimeFromString("15 Nov 2004 12:30:00 UTC"),
                             TestTimeFromString("16 Nov 2004 13:00:00 UTC")}));

  int index = summary_model_.AddColumn(std::string{kFormula});

  EXPECT_EQ(index, 2);

  EXPECT_CALL(*timed_data, RemoveObserver(_));
  EXPECT_CALL(*timed_data, RemoveViewObserver(_));

  return TestColumn{index, timed_data};
}

aui::GridCell SummaryModelTest::GetCellAt(int column_index, base::Time time) {
  int row_index = summary_model_.GetRowForTime(time);

  aui::GridCell cell{.row = row_index, .column = column_index};
  summary_model_.GetCell(cell);

  return cell;
}

TEST_F(SummaryModelTest, AddColumn) {
  AddColumn();
}

TEST_F(SummaryModelTest, Load) {
  EXPECT_EQ(kAggregateType, summary_model_.aggregate_type());
  EXPECT_EQ(kInterval, summary_model_.interval());
  EXPECT_EQ(kTimeRange, summary_model_.time_range());

  ASSERT_EQ(summary_model_.row_model().GetCount(), 49);  // 30-min interval
  EXPECT_EQ(summary_model_.GetRowTime(0),
            TestTimeFromString("15 Nov 2004 12:30:00 UTC"));
  EXPECT_EQ(summary_model_.GetRowTime(48),
            TestTimeFromString("16 Nov 2004 12:30:00 UTC"));
  EXPECT_EQ(48, summary_model_.GetRowForTime(
                    TestTimeFromString("16 Nov 2004 12:56:43 UTC")));
}

TEST_F(SummaryModelTest, Save) {
  WindowDefinition window_definition{kSummaryWindowInfo};
  summary_model_.Save(window_definition);
  EXPECT_EQ(kWindowDefinition, window_definition);
}

TEST_F(SummaryModelTest, CellsAreGreyWhileLoading) {
  auto test_column = AddColumn();

  std::vector<scada::DateTimeRange> ready_ranges{
      scada::DateTimeRange{TestTimeFromString("15 Nov 2004 14:00:00 UTC"),
                           TestTimeFromString("15 Nov 2004 15:00:00 UTC")}};

  ON_CALL(*test_column.timed_data, GetReadyRanges())
      .WillByDefault(ReturnRef(ready_ranges));

  EXPECT_EQ(aui::ColorCode::DarkGray,
            GetCellAt(test_column.index,
                      TestTimeFromString("15 Nov 2004 13:30:00 UTC"))
                .cell_color);

  EXPECT_EQ(aui::ColorCode::White,
            GetCellAt(test_column.index,
                      TestTimeFromString("15 Nov 2004 14:00:00 UTC"))
                .cell_color);

  EXPECT_EQ(aui::ColorCode::White,
            GetCellAt(test_column.index,
                      TestTimeFromString("15 Nov 2004 14:30:00 UTC"))
                .cell_color);

  EXPECT_EQ(aui::ColorCode::DarkGray,
            GetCellAt(test_column.index,
                      TestTimeFromString("15 Nov 2004 15:00:00 UTC"))
                .cell_color);
}
