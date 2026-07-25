#include "table_model.h"

#include "aui/dialog_service_mock.h"
#include "aui/severity_colors.h"
#include "aui/test/recording_table_model_observer.h"
#include "base/blinker_mock.h"
#include "base/observer_list.h"
#include "base/test/scoped_mock_clock_override.h"
#include "common/node_state.h"
#include "events/node_event_provider_mock.h"
#include "model/data_items_node_ids.h"
#include "modules/table/sparkline.h"
#include "modules/table/table_row.h"
#include "node_service/test/fake_node_service.h"
#include "profile/profile.h"
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
    scada::base::ObserverList<TimedDataObserver> observers;
    scada::base::ObserverList<TimedDataViewObserver> view_observers;
    StrictMock<MockTimedData> timed_data;
  };

  // NOTE: |TableRow| references |RowContext| via |TimedData|, so the
  // |RowContext| may outlive local reference.
  std::shared_ptr<RowContext> SetFormula();

  FakeNodeService node_service_;
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

  scada::aui::RecordingTableModelObserver table_model_observer_{table_model_};
};

namespace {

scada::aui::Color GetCellColor(const TableModel& table_model,
                               int row,
                               int column_id) {
  TableCellEx cell = {};
  cell.row = row;
  cell.column_id = column_id;
  table_model.GetCellEx(cell);
  return cell.cell_color;
}

scada::aui::Color GetTextColor(const TableModel& table_model,
                               int row,
                               int column_id) {
  TableCellEx cell = {};
  cell.row = row;
  cell.column_id = column_id;
  table_model.GetCellEx(cell);
  return cell.text_color;
}

NodeRef MakeDiscreteItemNode(FakeNodeService& node_service) {
  // The type has no supertype and the item no TsFormat reference, so both
  // resolve to a null NodeRef.
  node_service.Add(
      scada::NodeState{.node_id = scada::data_items::id::DiscreteItemType});

  return node_service.Add(scada::NodeState{
      .node_id = scada::NodeId{1, 1},
      .type_definition_id = scada::data_items::id::DiscreteItemType});
}

}  // namespace

TableModelTest::TableModelTest() {
  table_model_.item_changed_ = item_changed_.AsStdFunction();
}

TableModelTest::~TableModelTest() = default;

std::shared_ptr<TableModelTest::RowContext> TableModelTest::SetFormula() {
  auto row_context = std::make_shared<RowContext>();

  ON_CALL(row_context->timed_data, AddObserver(_))
      .WillByDefault(Invoke(
          [&observers = row_context->observers](TimedDataObserver& observer) {
            observers.AddObserver(&observer);
          }));

  ON_CALL(row_context->timed_data, AddViewObserver(_, _))
      .WillByDefault(Invoke(
          [&view_observers = row_context->view_observers](
              TimedDataViewObserver& observer, const scada::TimeRange& range) {
            view_observers.AddObserver(&observer);
          }));

  ON_CALL(row_context->timed_data, RemoveObserver(_))
      .WillByDefault(Invoke(
          [&observers = row_context->observers](TimedDataObserver& observer) {
            observers.RemoveObserver(&observer);
          }));

  ON_CALL(row_context->timed_data, RemoveViewObserver(_))
      .WillByDefault(Invoke([&view_observers = row_context->view_observers](
                                TimedDataViewObserver& observer) {
        view_observers.RemoveObserver(&observer);
      }));

  const std::string formula = "formula";
  EXPECT_CALL(
      timed_data_service_,
      GetFormulaTimedData(std::string_view{formula}, scada::AggregateFilter{}))
      .WillOnce(Return(
          std::shared_ptr<TimedData>{row_context, &row_context->timed_data}));

  EXPECT_CALL(row_context->timed_data, AddObserver(_));

  EXPECT_CALL(
      row_context->timed_data,
      AddViewObserver(_, scada::TimeRange{scada::kMaxTime, scada::kMaxTime}));

  EXPECT_CALL(row_context->timed_data, IsAlerting());

  const scada::NodeId node_id{1, 1};

  const NodeRef item_node =
      node_service_.Add(scada::NodeState{.node_id = node_id});

  ON_CALL(row_context->timed_data, GetNode()).WillByDefault(Return(item_node));

  const int row_index = table_model_.row_count();

  EXPECT_CALL(row_context->timed_data, GetNode());
  EXPECT_CALL(item_changed_, Call(node_id, true));

  table_model_observer_.ClearEvents();

  EXPECT_TRUE(table_model_.SetFormula(row_index, formula));

  EXPECT_THAT(table_model_observer_.items_adding,
              ElementsAre(Pair(row_index, 1)));
  EXPECT_THAT(table_model_observer_.items_added,
              ElementsAre(Pair(row_index, 1)));

  EXPECT_CALL(row_context->timed_data, RemoveObserver(_));
  EXPECT_CALL(row_context->timed_data, RemoveViewObserver(_));

  return row_context;
}

// Under the reshell theme a new row also observes a trailing history window
// feeding its sparkline cell; the legacy grid stays current-only (covered by
// the {Max, Max} view-observer expectation in the SetFormula helper).
TEST_F(TableModelTest, ReshellRowObservesTheSparklineWindow) {
  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kDark);
  const scada::base::ScopedMockClockOverride clock;

  struct NiceRowContext {
    NiceMock<MockTimedData> timed_data;
  };
  auto row_context = std::make_shared<NiceRowContext>();
  const std::string formula = "formula";
  EXPECT_CALL(
      timed_data_service_,
      GetFormulaTimedData(std::string_view{formula}, scada::AggregateFilter{}))
      .WillOnce(Return(
          std::shared_ptr<TimedData>{row_context, &row_context->timed_data}));

  table_model_.SetFormula(0, formula);

  const TableRow* row = table_model_.GetRow(0);
  ASSERT_NE(row, nullptr);
  EXPECT_EQ(row->timed_data().from(), scada::Now() - kSparklineWindow);

  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kLegacy);
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

TEST_F(TableModelTest, DiscreteOpenValueUsesPaletteTextColor) {
  const auto& row_context = SetFormula();

  EXPECT_CALL(row_context->timed_data, GetDataValue())
      .Times(AnyNumber())
      .WillRepeatedly(Return(scada::DataValue{false, {}, {}, {}}));
  EXPECT_CALL(row_context->timed_data, GetNode())
      .Times(AnyNumber())
      .WillRepeatedly(Return(MakeDiscreteItemNode(node_service_)));

  EXPECT_EQ(scada::aui::Color{scada::aui::ColorCode::Transparent},
            GetTextColor(table_model_, 0, TableModel::COLUMN_VALUE));
}

TEST_F(TableModelTest, ValueBlinking) {
  const auto& row_context = SetFormula();

  // + empty row.
  ASSERT_EQ(2, table_model_.GetRowCount());

  EXPECT_CALL(row_context->timed_data, GetDataValue()).Times(AnyNumber());
  EXPECT_CALL(row_context->timed_data, GetNode()).Times(AnyNumber());

  // Not alerting, not blinking.

  EXPECT_EQ(scada::aui::Color{scada::aui::ColorCode::Transparent},
            GetCellColor(table_model_, 0, TableModel::COLUMN_VALUE));

  // Alerting, but not blinking.

  EXPECT_CALL(row_context->timed_data, IsAlerting()).WillOnce(Return(true));

  table_model_observer_.ClearEvents();

  for (auto& o : row_context->observers)
    o.OnEventsChanged();

  EXPECT_THAT(table_model_observer_.items_changed, ElementsAre(Pair(0, 1)));

  EXPECT_CALL(blinker_manager_, GetState()).WillOnce(Return(false));
  EXPECT_EQ(scada::aui::Color{scada::aui::ColorCode::Transparent},
            GetCellColor(table_model_, 0, TableModel::COLUMN_VALUE));

  // Alerting and blinking.

  EXPECT_CALL(blinker_manager_, GetState()).WillOnce(Return(true));
  EXPECT_EQ(scada::aui::Color{scada::aui::ColorCode::Yellow},
            GetCellColor(table_model_, 0, TableModel::COLUMN_VALUE));
}
