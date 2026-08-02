#include "modules/sheet/sheet_model.h"

#include "modules/sheet/sheet_cell.h"
#include "profile/window_definition.h"
#include "timed_data/timed_data_service_fake.h"

#include <gtest/gtest.h>

#include <memory>

namespace {

// A blinker that is always in its "on" phase and never fires. The sheet only
// needs a manager to register with; nothing here depends on the blink cycle.
class FakeBlinkerManager : public BlinkerManager {
 public:
  virtual bool GetState() const override { return true; }

  virtual boost::signals2::scoped_connection Subscribe(
      const BlinkerCallback& callback) override {
    return {};
  }
};

// A sheet holding one cell bound to `formula`, over a service that reports
// that formula as alerting — the state that puts the cell in the model's
// blinking set.
class SheetModelTest : public testing::Test {
 protected:
  std::unique_ptr<SheetModel> MakeAlertingSheet(const std::string& formula) {
    auto timed_data = timed_data_service_.AddTimedData(formula);
    timed_data->alerting = true;

    auto model = std::make_unique<SheetModel>(SheetModelContext{
        .timed_data_service_ = timed_data_service_,
        .blinker_manager_ = blinker_manager_,
    });
    model->SetSizes(10, 10);

    WindowDefinition definition{"CusTable"};
    definition.AddItem("SheetCell")
        .SetInt("row", 1)
        .SetInt("col", 1)
        .SetString("text", "=" + formula);
    model->Load(definition);

    return model;
  }

  FakeTimedDataService timed_data_service_;
  FakeBlinkerManager blinker_manager_;
};

TEST_F(SheetModelTest, LoadsAFormulaCell) {
  auto model = MakeAlertingSheet("TIT.200");

  scada::aui::GridCell cell{.row = 0, .column = 0};
  model->GetCell(cell);

  // The cell blinks, which is what the destruction test below depends on.
  EXPECT_EQ(cell.cell_color, scada::aui::ColorCode::Yellow);
}

// Regression: a formula cell cached the text it formatted when the sheet
// opened. The item's display format is fetched asynchronously and nothing
// notifies when it arrives, so a sheet opened before its nodes loaded rendered
// unformatted numbers for the life of the window — visible as a capture that
// differed between a full generator run and `--only sheet.png`.
TEST_F(SheetModelTest, FormulaCellFormatsItsValueOnRead) {
  auto timed_data = timed_data_service_.AddTimedData("TIT.200");
  timed_data->current = scada::DataValue{
      scada::Variant{1.0}, {}, scada::kNullTime, scada::kNullTime};

  FakeBlinkerManager blinker_manager;
  SheetModel model{SheetModelContext{
      .timed_data_service_ = timed_data_service_,
      .blinker_manager_ = blinker_manager,
  }};
  model.SetSizes(10, 10);

  WindowDefinition definition{"CusTable"};
  definition.AddItem("SheetCell")
      .SetInt("row", 1)
      .SetInt("col", 1)
      .SetString("text", "=TIT.200");
  model.Load(definition);

  // The value changes with no notification of any kind — the cell must still
  // report it, because it formats on read.
  timed_data->current = scada::DataValue{
      scada::Variant{2.0}, {}, scada::kNullTime, scada::kNullTime};

  scada::aui::GridCell cell{.row = 0, .column = 0};
  model.GetCell(cell);
  EXPECT_EQ(cell.text, u"2");
}

// Regression: a cell's alignment is parsed from the saved window, kept in its
// format and offered in the context menu, but the model never handed it to the
// grid — GridCell had nowhere to put it — so a right-aligned or centred cell
// rendered left-aligned.
TEST_F(SheetModelTest, ReportsTheCellsStoredAlignment) {
  FakeBlinkerManager blinker_manager;
  SheetModel model{SheetModelContext{
      .timed_data_service_ = timed_data_service_,
      .blinker_manager_ = blinker_manager,
  }};
  model.SetSizes(10, 10);

  WindowDefinition definition{"CusTable"};
  definition.AddItem("SheetCell")
      .SetInt("row", 1)
      .SetInt("col", 1)
      .SetString("text", "Итого")
      .SetString("align", "right");
  definition.AddItem("SheetCell")
      .SetInt("row", 1)
      .SetInt("col", 2)
      .SetString("text", "42")
      .SetString("align", "center");
  definition.AddItem("SheetCell")
      .SetInt("row", 2)
      .SetInt("col", 1)
      .SetString("text", "Без выравнивания");
  model.Load(definition);

  scada::aui::GridCell right{.row = 0, .column = 0};
  model.GetCell(right);
  EXPECT_EQ(right.alignment, scada::aui::TableColumn::RIGHT);

  scada::aui::GridCell center{.row = 0, .column = 1};
  model.GetCell(center);
  EXPECT_EQ(center.alignment, scada::aui::TableColumn::CENTER);

  scada::aui::GridCell left{.row = 1, .column = 0};
  model.GetCell(left);
  EXPECT_EQ(left.alignment, scada::aui::TableColumn::LEFT);
}

// Regression: ~SheetCell erases itself from SheetModel::blinking_cells_, so
// that set has to outlive the cells. It was declared after them, hence
// destroyed first, and closing a custom table holding a blinking cell
// segfaulted inside std::set::erase — reachable from any sheet whose cells are
// bound to live values.
TEST_F(SheetModelTest, DestroyingASheetWithABlinkingCellDoesNotUseFreedMemory) {
  auto model = MakeAlertingSheet("TIT.200");
  model.reset();
}

}  // namespace
