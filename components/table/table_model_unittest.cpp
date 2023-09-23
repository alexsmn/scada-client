#include "table_model.h"

#include "aui/models/table_model_observer_mock.h"
#include "base/blinker_mock.h"
#include "base/observer_list.h"
#include "events/node_event_provider_mock.h"
#include "node_service/test/test_node_model.h"
#include "services/dialog_service_mock.h"
#include "services/profile.h"
#include "timed_data/timed_data_mock.h"
#include "timed_data/timed_data_observer.h"
#include "timed_data/timed_data_service_mock.h"

#include <gmock/gmock.h>

using namespace testing;

class TableModelTest : public Test {
 public:
  TableModelTest();
  ~TableModelTest();

 protected:
  struct RowContext {
    base::ObserverList<TimedDataObserver> observers;
    StrictMock<MockTimedData> timed_data;
  };

  // NOTE: |TableRow| references |RowContext| via |TimedData|, so the
  // |RowContext| may outlive local reference.
  std::shared_ptr<RowContext> SetFormula();

  StrictMock<MockTimedDataService> timed_data_service_;
  StrictMock<MockNodeEventProvider> node_event_provider_;
  const Profile profile_;
  StrictMock<MockDialogService> dialog_service_;
  NiceMock<MockBlinkerManager> blinker_manager_;
  StrictMock<MockFunction<void(const scada::NodeId& item_id, bool added)>>
      item_changed_;
  StrictMock<aui::TableModelObserverMock> table_model_observer_;

  TableModel table_model_{TableModelContext{timed_data_service_,
                                            node_event_provider_, profile_,
                                            dialog_service_, blinker_manager_}};
};

namespace {

aui::Color GetCellColor(const TableModel& table_model, int row, int column_id) {
  TableCellEx cell = {};
  cell.row = row;
  cell.column_id = column_id;
  table_model.GetCellEx(cell);
  return cell.cell_color;
}

}  // namespace

TableModelTest::TableModelTest() {
  table_model_.item_changed_ = item_changed_.AsStdFunction();

  table_model_.observers().AddObserver(&table_model_observer_);
}

TableModelTest::~TableModelTest() {
  table_model_.observers().RemoveObserver(&table_model_observer_);
}

std::shared_ptr<TableModelTest::RowContext> TableModelTest::SetFormula() {
  auto row_context = std::make_shared<RowContext>();

  ON_CALL(row_context->timed_data, AddObserver(_, _))
      .WillByDefault(Invoke(
          [&observers = row_context->observers](
              TimedDataObserver& observer, const scada::DateTimeRange& range) {
            observers.AddObserver(&observer);
          }));

  ON_CALL(row_context->timed_data, RemoveObserver(_))
      .WillByDefault(Invoke(
          [&observers = row_context->observers](TimedDataObserver& observer) {
            observers.RemoveObserver(&observer);
          }));

  const std::string formula = "formula";
  EXPECT_CALL(
      timed_data_service_,
      GetFormulaTimedData(std::string_view{formula}, scada::AggregateFilter{}))
      .WillOnce(Return(
          std::shared_ptr<TimedData>{row_context, &row_context->timed_data}));

  EXPECT_CALL(row_context->timed_data,
              AddObserver(_, scada::DateTimeRange{scada::DateTime::Max(),
                                                  scada::DateTime::Max()}));

  EXPECT_CALL(row_context->timed_data, IsAlerting());

  const scada::NodeId node_id{1, 1};

  ON_CALL(row_context->timed_data, GetNode())
      .WillByDefault(Return(
          std::make_shared<TestNodeModel>(nullptr, scada::NodeState{node_id})));

  const int row_index = table_model_.row_count();

  EXPECT_CALL(row_context->timed_data, GetNode());
  EXPECT_CALL(table_model_observer_, OnItemsAdding(row_index, 1));
  EXPECT_CALL(table_model_observer_, OnItemsAdded(row_index, 1));
  EXPECT_CALL(item_changed_, Call(node_id, true));

  EXPECT_TRUE(table_model_.SetFormula(row_index, formula));

  EXPECT_CALL(row_context->timed_data, RemoveObserver(_));

  return row_context;
}

TEST_F(TableModelTest, SetFormula) {
  SetFormula();
  SetFormula();
  SetFormula();

  // + empty row.
  ASSERT_EQ(4, table_model_.GetRowCount());
}

TEST_F(TableModelTest, GetTitle) {
  const auto& row_context = SetFormula();

  // + empty row.
  ASSERT_EQ(2, table_model_.GetRowCount());

  const std::u16string title = u"Title";
  EXPECT_CALL(row_context->timed_data, GetTitle()).WillOnce(Return(title));
  // For icon index.
  EXPECT_CALL(row_context->timed_data, GetNode());
  EXPECT_EQ(title, table_model_.GetCellText(0, TableModel::COLUMN_TITLE));
}

TEST_F(TableModelTest, GetValue) {
  const auto& row_context = SetFormula();

  // + empty row.
  ASSERT_EQ(2, table_model_.GetRowCount());

  EXPECT_CALL(row_context->timed_data, GetNode()).Times(AnyNumber());
  const scada::LocalizedText value = u"value";
  const scada::DataValue data_value{value, {}, {}, {}};
  EXPECT_CALL(row_context->timed_data, GetDataValue())
      .Times(AnyNumber())
      .WillRepeatedly(Return(data_value));
  EXPECT_EQ(value, table_model_.GetCellText(0, TableModel::COLUMN_VALUE));
}

TEST_F(TableModelTest, ValueBlinking) {
  const auto& row_context = SetFormula();

  // + empty row.
  ASSERT_EQ(2, table_model_.GetRowCount());

  EXPECT_CALL(row_context->timed_data, GetDataValue()).Times(AnyNumber());
  EXPECT_CALL(row_context->timed_data, GetNode()).Times(AnyNumber());

  // Not alerting, not blinking.

  EXPECT_EQ(aui::Color{aui::ColorCode::Transparent},
            GetCellColor(table_model_, 0, TableModel::COLUMN_VALUE));

  // Alerting, but not blinking.

  EXPECT_CALL(row_context->timed_data, IsAlerting()).WillOnce(Return(true));
  EXPECT_CALL(table_model_observer_, OnItemsChanged(0, 1));

  for (auto& o : row_context->observers)
    o.OnEventsChanged();

  EXPECT_CALL(blinker_manager_, GetState()).WillOnce(Return(false));
  EXPECT_EQ(aui::Color{aui::ColorCode::Transparent},
            GetCellColor(table_model_, 0, TableModel::COLUMN_VALUE));

  // Alerting and blinking.

  EXPECT_CALL(blinker_manager_, GetState()).WillOnce(Return(true));
  EXPECT_EQ(aui::Color{aui::ColorCode::Yellow},
            GetCellColor(table_model_, 0, TableModel::COLUMN_VALUE));
}
