#include "table_model.h"

#include "aui/dialog_service_mock.h"
#include "aui/severity_colors.h"
#include "aui/test/recording_table_model_observer.h"
#include "base/blinker_mock.h"
#include "base/observer_list.h"
#include "base/test/awaitable_test.h"
#include "base/test/scoped_mock_clock_override.h"
#include "base/test/test_executor.h"
#include "base/time/calendar.h"
#include "common/node_state.h"
#include "events/node_event_provider_mock.h"
#include "model/data_items_node_ids.h"
#include "modules/table/quality_mark.h"
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

  TestExecutor executor_;
  FakeNodeService node_service_;
  StrictMock<MockTimedDataService> timed_data_service_;
  StrictMock<MockNodeEventProvider> node_event_provider_;
  const Profile profile_;
  StrictMock<MockDialogService> dialog_service_;
  NiceMock<MockBlinkerManager> blinker_manager_;
  StrictMock<MockFunction<void(const scada::NodeId& item_id, bool added)>>
      item_changed_;

  TableModel table_model_{TableModelContext{executor_, timed_data_service_,
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

// The Source column is what tells an engineer where a value comes from — the
// job the row icon used to do by its own presence, silently and without a
// label (docs/product/ui-mockups/screens/table-watch.html gives it a column).
TEST_F(TableModelTest, SourceColumnShowsWhatTheRowIsBoundTo) {
  const auto& row_context = SetFormula();
  // Cell rendering consults the row's current value for its colours.
  EXPECT_CALL(row_context->timed_data, GetDataValue()).Times(AnyNumber());

  // The leading "=" is how the client spells a computed row, which is exactly
  // the distinction this column exists to make visible: an expression, not a
  // NodeId.
  EXPECT_EQ(u"=formula",
            table_model_.GetCellText(0, TableModel::COLUMN_SOURCE));
}

// The trailing empty row is bound to nothing, and says so by being empty
// rather than by lacking a mark.
TEST_F(TableModelTest, SourceColumnIsEmptyForTheUnboundRow) {
  ASSERT_EQ(1, table_model_.GetRowCount());

  EXPECT_EQ(u"", table_model_.GetCellText(0, TableModel::COLUMN_SOURCE));
}

// Every row is a data item, so a kind glyph distinguishes nothing; the one
// this used to draw encoded state through its presence, which principles.md §5
// rules out.
TEST_F(TableModelTest, TitleCellCarriesNoIcon) {
  const auto& row_context = SetFormula();
  EXPECT_CALL(row_context->timed_data, GetDataValue()).Times(AnyNumber());

  const std::u16string title = u"Title";
  EXPECT_CALL(row_context->timed_data, GetTitle()).WillOnce(Return(title));

  TableCellEx cell = {};
  cell.row = 0;
  cell.column_id = TableModel::COLUMN_TITLE;
  table_model_.GetCellEx(cell);

  EXPECT_EQ(cell.icon_index, -1);
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
  // No GetNode() expectation: the title cell used to ask for the row's node
  // only to decide whether to draw an icon, and it no longer draws one.
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

// A value that carries no timestamp must leave the timestamp columns blank.
// The delivery path used to hand the table a default-constructed scada::Time
// — the Unix epoch under std::chrono, which scada::IsNull() does not
// recognise — and the cells faithfully formatted it as a 1969/1970 date.
// A fabricated timestamp beside a live value is exactly the honesty failure
// docs/client/ux/principles.md §5 forbids, and reads worse than a blank because
// an operator takes it for real.
TEST_F(TableModelTest, MissingTimestampsRenderBlank) {
  const auto& row_context = SetFormula();

  EXPECT_CALL(row_context->timed_data, GetNode()).Times(AnyNumber());
  EXPECT_CALL(row_context->timed_data, GetDataValue())
      .Times(AnyNumber())
      .WillRepeatedly(Return(scada::DataValue{
          42.0, scada::Qualifier{}, scada::kNullTime, scada::kNullTime}));
  EXPECT_CALL(row_context->timed_data, GetChangeTime())
      .Times(AnyNumber())
      .WillRepeatedly(Return(scada::kNullTime));

  EXPECT_EQ(table_model_.GetCellText(0, TableModel::COLUMN_SOURCE_TIMESTAMP),
            u"");
  EXPECT_EQ(table_model_.GetCellText(0, TableModel::COLUMN_SERVER_TIMESTAMP),
            u"");
  EXPECT_EQ(table_model_.GetCellText(0, TableModel::COLUMN_CHANGE_TIME), u"");
}

// A row that never received a reading — its formula resolves to a node the
// server does not have — must not date the value it does not have. Before the
// fix the delivery path stamped its own "now" onto the failed read, so the row
// showed "Нет данных" quality and an empty Value cell beside two fully
// plausible timestamps.
TEST_F(TableModelTest, UndeliveredRowRendersNoTimestamps) {
  const auto& row_context = SetFormula();

  const scada::Time stamp =
      scada::base::TimeFromString("2026-04-16 15:02:00", /*is_local=*/true)
          .value_or(scada::kNullTime);
  ASSERT_FALSE(scada::IsNull(stamp));

  // Nothing delivered: null value and a default (zero) Qualifier, but carrying
  // the timestamps the local delivery path stamps on a failed read.
  const scada::DataValue undelivered{scada::Variant{}, scada::Qualifier{},
                                     stamp, stamp};
  ASSERT_FALSE(QualityFromValue(undelivered).has_value())
      << "fixture must model a row that never received a reading";

  EXPECT_CALL(row_context->timed_data, GetNode()).Times(AnyNumber());
  EXPECT_CALL(row_context->timed_data, GetDataValue())
      .Times(AnyNumber())
      .WillRepeatedly(Return(undelivered));
  EXPECT_CALL(row_context->timed_data, GetChangeTime())
      .Times(AnyNumber())
      .WillRepeatedly(Return(stamp));

  EXPECT_EQ(table_model_.GetCellText(0, TableModel::COLUMN_SOURCE_TIMESTAMP),
            u"");
  EXPECT_EQ(table_model_.GetCellText(0, TableModel::COLUMN_SERVER_TIMESTAMP),
            u"");
  EXPECT_EQ(table_model_.GetCellText(0, TableModel::COLUMN_CHANGE_TIME), u"");
}

// The complement: a real timestamp still renders.
TEST_F(TableModelTest, DeliveredTimestampsRender) {
  const auto& row_context = SetFormula();

  const scada::Time stamp =
      scada::base::TimeFromString("2026-04-16 15:02:00", /*is_local=*/true)
          .value_or(scada::kNullTime);
  ASSERT_FALSE(scada::IsNull(stamp));

  EXPECT_CALL(row_context->timed_data, GetNode()).Times(AnyNumber());
  EXPECT_CALL(row_context->timed_data, GetDataValue())
      .Times(AnyNumber())
      .WillRepeatedly(
          Return(scada::DataValue{42.0, scada::Qualifier{}, stamp, stamp}));
  EXPECT_CALL(row_context->timed_data, GetChangeTime())
      .Times(AnyNumber())
      .WillRepeatedly(Return(stamp));

  for (int column :
       {TableModel::COLUMN_SOURCE_TIMESTAMP,
        TableModel::COLUMN_SERVER_TIMESTAMP, TableModel::COLUMN_CHANGE_TIME}) {
    const std::u16string text = table_model_.GetCellText(0, column);
    EXPECT_NE(text.find(u"2026"), std::u16string::npos)
        << "column " << column << " rendered "
        << std::string{text.begin(), text.end()};
  }
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

// A lazy awaitable that records only when it is actually awaited. Asserting
// that the mock was *called* would not catch this bug: the call happens either
// way, and only awaiting the returned awaitable runs the coroutine body that
// shows the box. Same shape as `CompleteLazily` in
// `modules/configuration/configuration_module_unittest.cpp`.
namespace {

Awaitable<MessageBoxResult> ShowLazily(bool* shown) {
  *shown = true;
  co_return MessageBoxResult::Ok;
}

}  // namespace

// Regression: `SetCellText` reported an invalid formula with a bare
// `RunMessageBox(...)`. That returns a lazy awaitable, so the discarded
// coroutine never ran and no box appeared — the cell edit was rejected in
// total silence, which reads to an operator as a dead grid.
TEST_F(TableModelTest, InvalidFormulaShowsMessageBox) {
  EXPECT_CALL(timed_data_service_, GetFormulaTimedData(_, _))
      .WillOnce(Throw(std::runtime_error{"bad formula"}));

  bool shown = false;
  EXPECT_CALL(dialog_service_, RunMessageBox(_, _, MessageBoxMode::Error))
      .WillOnce(Invoke([&](std::u16string_view, std::u16string_view,
                           MessageBoxMode) { return ShowLazily(&shown); }));

  EXPECT_FALSE(
      table_model_.SetCellText(0, TableModel::COLUMN_TITLE, u"=nonsense"));
  Drain(executor_);

  EXPECT_TRUE(shown);
}
