#include "components/timed_data/timed_data_model.h"

#include "base/test/simple_test_clock.h"
#include "base/test/test_time.h"
#include "profile/window_definition.h"
#include "timed_data/timed_data_service_fake.h"

#include <gmock/gmock.h>

using namespace testing;

class TimedDataModelTest : public Test {
 protected:
  base::SimpleTestClock clock_;
  FakeTimedDataService timed_data_service_;

  TimedDataModel model_{
      {.clock_ = clock_, .timed_data_service_ = timed_data_service_}};
};

TEST_F(TimedDataModelTest, ShowsDataForTheDayByDefault) {
  clock_.SetNow(TestTimeFromString("15 Nov 2004 10:22:00"));

  auto timed_data = timed_data_service_.AddTimedData("item1");

  for (auto timestamp = TestTimeFromString("14 Nov 2004 11:11:11");
       timestamp <= clock_.Now();
       timestamp += base::TimeDelta::FromMinutes(30)) {
    timed_data->data_values.emplace_back(/*value=*/timestamp.ToInternalValue(),
                                         /*qualifier=*/scada::Qualifier{},
                                         /*source_timestamp=*/timestamp,
                                         /*server_timestamp=*/timestamp);
  }

  // ACT
  model_.Init(WindowDefinition{}.AddItem(
      std::move(WindowItem{"Item"}.SetString("path", "item1"))));

  EXPECT_EQ(model_.GetTimeRange(), TimeRange{TimeRange::Type::Day});
  ASSERT_NE(model_.GetRowCount(), 0);
  EXPECT_EQ(model_.GetCellText(/*row=*/0, TimedDataModel::CID_TIME),
            u"15.11.2004 00:11:11.000");
  EXPECT_EQ(model_.GetCellText(/*row=*/model_.GetRowCount() - 1,
                               TimedDataModel::CID_TIME),
            u"15.11.2004 10:11:11.000");
}