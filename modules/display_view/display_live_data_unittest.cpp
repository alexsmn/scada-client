#include "display_view/display_live_data.h"

#include "timed_data/timed_data_service_mock.h"
#include "timed_data/timed_data_spec.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

using testing::Return;

// What a schematic's value field is sent when there is nothing to show. It
// must never be empty: an empty text left the runtime to keep whatever the
// field last showed, which for a field that never got a reading is the
// designer's saved number (backlog 1005).

TEST(DisplayLiveDataTest, AnUnconnectedSourceReadsAsBadWithTheMark) {
  const TimedDataSpec spec;
  const DisplayDataSourceReading reading =
      DisplayLiveData::MakeReading("TIT_U", spec);

  EXPECT_EQ(reading.data_source, "TIT_U");
  EXPECT_FALSE(reading.value.has_value());
  EXPECT_EQ(reading.text, "?");
  EXPECT_NE(reading.quality, 0);
}

TEST(DisplayLiveDataTest, AConnectedSourceWithNoValueYetShowsTheMark) {
  testing::NiceMock<MockTimedDataService> service;
  TimedDataSpec spec;
  spec.Connect(service, "TIT_U");
  ASSERT_TRUE(spec.connected());
  ON_CALL(*service.default_timed_data_, GetDataValue())
      .WillByDefault(Return(scada::DataValue{}));

  const DisplayDataSourceReading reading =
      DisplayLiveData::MakeReading("TIT_U", spec);

  EXPECT_FALSE(reading.value.has_value());
  EXPECT_EQ(reading.text, "?");
  EXPECT_TRUE(reading.state.empty());
}

TEST(DisplayLiveDataTest, AValueIsSentAsANumberAndText) {
  testing::NiceMock<MockTimedDataService> service;
  TimedDataSpec spec;
  spec.Connect(service, "TIT_U");
  scada::DataValue value;
  value.value = 12.5;
  ON_CALL(*service.default_timed_data_, GetDataValue())
      .WillByDefault(Return(value));

  const DisplayDataSourceReading reading =
      DisplayLiveData::MakeReading("TIT_U", spec);

  ASSERT_TRUE(reading.value.has_value());
  EXPECT_DOUBLE_EQ(*reading.value, 12.5);
  EXPECT_FALSE(reading.text.empty());
  EXPECT_NE(reading.text, "?");
  EXPECT_EQ(reading.quality, 0);
}

}  // namespace
