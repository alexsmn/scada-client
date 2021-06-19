#include "components/summary/summary_model.h"

#include "common_resources.h"
#include "node_service/node_service_mock.h"
#include "timed_data/timed_data_service_mock.h"
#include "window_definition.h"

#include <gmock/gmock.h>

using namespace testing;

class SummaryModelTest : public Test {
 public:
  SummaryModelTest();

 protected:
  const WindowDefinition kWindowDefinition =
      WindowDefinition{GetWindowInfo(ID_SUMMARY_VIEW)}.AddItem(
          std::move(WindowItem{"AggregateType"}.Set(kAggregateType)));

  MockNodeService node_service_;
  MockTimedDataService timed_data_service_;
  SummaryModel summary_model_{
      SummaryModelContext{node_service_, timed_data_service_}};

  inline static const auto kAggregateType =
      scada::id::AggregateFunction_Maximum;
};

SummaryModelTest::SummaryModelTest() {
  summary_model_.Load(kWindowDefinition);
}

TEST_F(SummaryModelTest, Load) {
  EXPECT_EQ(kAggregateType, summary_model_.aggregate_type());
}

TEST_F(SummaryModelTest, Save) {
  WindowDefinition window_definition{GetWindowInfo(ID_SUMMARY_VIEW)};
  summary_model_.Save(window_definition);
  const auto kExpectedWindowDefinition =
      WindowDefinition{GetWindowInfo(ID_SUMMARY_VIEW)}
          .AddItem(std::move(WindowItem{"TimeRange"}.SetString("type", "Day")))
          .AddItem(std::move(WindowItem{"Interval"}.SetInt("hours", 1)))
          .AddItem(std::move(WindowItem{"AggregateType"}.Set(kAggregateType)));
  EXPECT_EQ(kExpectedWindowDefinition, window_definition);
}
