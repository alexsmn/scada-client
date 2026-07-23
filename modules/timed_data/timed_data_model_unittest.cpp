#include "modules/timed_data/timed_data_model.h"
#include "base/time/time_wire_codec.h"

#include "base/test/simple_test_clock.h"
#include "base/test/test_time.h"
#include "profile/window_definition.h"
#include "profile/window_definition_util.h"
#include "timed_data/base_timed_data.h"
#include "timed_data/timed_data_property.h"
#include "timed_data/timed_data_service_fake.h"

#include <gmock/gmock.h>

using namespace testing;

class TimedDataModelTest : public Test {
 protected:
  scada::base::SimpleTestClock clock_;
  FakeTimedDataService timed_data_service_;

  TimedDataModel model_{
      {.clock_ = clock_, .timed_data_service_ = timed_data_service_}};
};

// A real BaseTimedData the test feeds live current-value updates through, so
// the model sees the same buffered-insert + property-change notification path
// production uses.
class LiveTimedData : public BaseTimedData {
 public:
  LiveTimedData() { historical_ = true; }

  void PushCurrent(scada::base::Time timestamp, double value) {
    scada::DataValue data_value{value, scada::Qualifier{},
                                /*source_timestamp=*/timestamp,
                                /*server_timestamp=*/timestamp};
    if (UpdateCurrent(data_value))
      NotifyPropertyChanged(PropertySet{PROPERTY_CURRENT});
  }

  std::string GetFormula(bool aliases) const override { return "item1"; }
  scada::LocalizedText GetTitle() const override { return {}; }
};

TEST_F(TimedDataModelTest, ShowsDataForTheDayByDefault) {
  clock_.SetNow(TestTimeFromString("15 Nov 2004 10:22:00"));

  auto timed_data = timed_data_service_.AddTimedData("item1");

  for (auto timestamp = TestTimeFromString("14 Nov 2004 11:11:11");
       timestamp <= clock_.Now();
       timestamp += std::chrono::minutes(30)) {
    timed_data->data_values.emplace_back(/*value=*/scada::base::EncodeWireMicroseconds(timestamp),
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
// Regression: a live current-value update whose timestamp lies OUTSIDE the
// view's (frozen, past) window used to trip TableModel::NotifyItemsChanged's
// count > 0 check (SIGTRAP): the changed range clamps to an empty span when
// nothing inside the window changed. Seen in the client/server E2E, which
// opens the timed-data view over a pre-launch window while live updates keep
// arriving.
TEST_F(TimedDataModelTest, LiveUpdateOutsideFrozenWindowIsIgnored) {
  clock_.SetNow(TestTimeFromString("15 Nov 2004 10:22:00"));

  auto timed_data = std::make_shared<LiveTimedData>();
  timed_data_service_.formulas["item1"] = timed_data;
  for (auto timestamp = TestTimeFromString("15 Nov 2004 09:00:00");
       timestamp <= TestTimeFromString("15 Nov 2004 09:30:00");
       timestamp += std::chrono::minutes(10)) {
    timed_data->PushCurrent(timestamp, /*value=*/1.0);
  }

  WindowDefinition definition;
  definition.AddItem("Item").SetString("path", "item1");
  SaveTimeRange(definition,
                TimeRange{TestTimeFromString("15 Nov 2004 09:00:00"),
                          TestTimeFromString("15 Nov 2004 09:40:00")});
  model_.Init(definition);
  const int rows = model_.GetRowCount();
  ASSERT_GT(rows, 0);

  // ACT: live values land after the window's end. The second one is the
  // crashing case — with two buffered values beyond the window, the changed
  // range's lower bound clamps past the visible rows and the notified count
  // went negative.
  timed_data->PushCurrent(TestTimeFromString("15 Nov 2004 10:00:00"),
                          /*value=*/2.0);
  timed_data->PushCurrent(TestTimeFromString("15 Nov 2004 10:01:00"),
                          /*value=*/3.0);

  EXPECT_EQ(model_.GetRowCount(), rows);
}
