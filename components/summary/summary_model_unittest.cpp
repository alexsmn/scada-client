#include "components/summary/summary_model.h"

#include "base/test/test_time.h"
#include "common/aggregation.h"
#include "common_resources.h"
#include "components/summary/summary_component.h"
#include "node_service/node_service_mock.h"
#include "timed_data/timed_data_mock.h"
#include "timed_data/timed_data_service_mock.h"
#include "window_definition.h"
#include "window_info.h"

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

TEST_F(SummaryModelTest, AddColumn) {
  const std::string_view kFormula = "test-formula";

  auto timed_data = std::make_shared<StrictMock<MockTimedData>>();

  EXPECT_CALL(
      timed_data_service_,
      GetFormulaTimedData(
          kFormula, scada::AggregateFilter{scada::GetLocalAggregateStartTime(),
                                           kInterval, kAggregateType}))
      .WillOnce(Return(timed_data));

  // Start time is rounded down, end time is rounded up.
  EXPECT_CALL(
      *timed_data,
      AddObserver(_, scada::DateTimeRange{
                         TestTimeFromString("15 Nov 2004 12:30:00 UTC"),
                         TestTimeFromString("16 Nov 2004 13:00:00 UTC")}));

  EXPECT_CALL(*timed_data, GetNode());

  int index = summary_model_.AddColumn(std::string{kFormula});

  EXPECT_EQ(index, 2);

  EXPECT_CALL(*timed_data, RemoveObserver(_));
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
