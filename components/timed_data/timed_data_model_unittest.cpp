#include "components/timed_data/timed_data_model.h"

#include "timed_data/timed_data_service_mock.h"

#include <gmock/gmock.h>

using namespace testing;

class TimedDataModelTest : public Test {
 protected:
  StrictMock<MockTimedDataService> timed_data_service_;

  TimedDataModel model_{{.timed_data_service_ = timed_data_service_}};
};

TEST_F(TimedDataModelTest, Test) {
  EXPECT_CALL(timed_data_service_, GetFormulaTimedData("abc", _));

  model_.SetFormula("abc");
}