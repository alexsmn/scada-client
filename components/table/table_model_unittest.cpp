#include "table_model.h"

#include "base/blinker_mock.h"
#include "common/node_event_provider_mock.h"
#include "node_service/test/test_node_model.h"
#include "services/dialog_service_mock.h"
#include "services/profile.h"
#include "timed_data/timed_data_mock.h"
#include "timed_data/timed_data_service_mock.h"

#include <gmock/gmock.h>

using namespace testing;

class TableModelTest : public Test {
 public:
  TableModelTest();

 protected:
  StrictMock<MockTimedDataService> timed_data_service_;
  StrictMock<MockNodeEventProvider> node_event_provider_;
  const Profile profile_;
  StrictMock<MockDialogService> dialog_service_;
  NiceMock<MockBlinkerManager> blinker_manager_;
  StrictMock<MockFunction<void(const scada::NodeId& item_id, bool added)>>
      item_changed_;

  TableModel table_model_{TableModelContext{timed_data_service_,
                                            node_event_provider_, profile_,
                                            dialog_service_, blinker_manager_}};
};

TableModelTest::TableModelTest() {
  table_model_.item_changed_ = item_changed_.AsStdFunction();
}

TEST_F(TableModelTest, SetFormula) {
  auto timed_data = std::make_shared<StrictMock<MockTimedData>>();

  const std::string formula = "formula";
  EXPECT_CALL(
      timed_data_service_,
      GetFormulaTimedData(std::string_view{formula}, scada::AggregateFilter{}))
      .WillOnce(Return(timed_data));

  EXPECT_CALL(*timed_data,
              AddObserver(_, scada::DateTimeRange{scada::DateTime::Max(),
                                                  scada::DateTime::Max()}));

  EXPECT_CALL(*timed_data, GetEvents());

  const scada::NodeId node_id{1, 1};

  ON_CALL(*timed_data, GetNode())
      .WillByDefault(Return(
          std::make_shared<TestNodeModel>(nullptr, scada::NodeState{node_id})));

  EXPECT_CALL(*timed_data, GetNode());
  EXPECT_CALL(item_changed_, Call(node_id, true));

  table_model_.SetFormula(-1, formula);

  // + empty row.
  ASSERT_EQ(2, table_model_.GetRowCount());

  const base::string16 title = L"Title";
  EXPECT_CALL(*timed_data, GetTitle()).WillOnce(Return(title));
  EXPECT_EQ(title, table_model_.GetCellText(0, TableModel::COLUMN_TITLE));

  EXPECT_CALL(*timed_data, GetNode()).Times(AnyNumber());
  const scada::LocalizedText value = L"value";
  const scada::DataValue data_value{value, {}, {}, {}};
  EXPECT_CALL(*timed_data, GetDataValue())
      .Times(AnyNumber())
      .WillRepeatedly(Return(data_value));
  EXPECT_EQ(value, table_model_.GetCellText(0, TableModel::COLUMN_VALUE));

  EXPECT_CALL(*timed_data, RemoveObserver(_));
}
