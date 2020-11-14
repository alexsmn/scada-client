#pragma once

#include <memory>

#include "contents_model.h"
#include "controller.h"
#include "timed_data/timed_data_spec.h"
#include "ui/base/models/fixed_row_model.h"
#include "ui/base/models/grid_model.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/grid/grid_controller.h"

class CellModel : public ui::GridModel, private views::FixedRowModel::Delegate {
 public:
  CellModel(TimedDataService& timed_data_service,
            DialogService& dialog_service);
  ~CellModel();

  CellModel(const CellModel&) = delete;
  CellModel& operator=(const CellModel&) = delete;

  int row_count() const { return row_count_; }
  int column_count() const { return column_count_; }

  views::FixedRowModel& row_model() { return row_model_; }
  ui::ColumnHeaderModel& column_model() { return column_model_; }

  class Cell {
   public:
    Cell(CellModel& model, int row, int column);

    Cell(const Cell&) = delete;
    Cell& operator=(const Cell&) = delete;

    CellModel& model_;
    const int row_;
    const int column_;

    TimedDataSpec value_spec_;
  };

  Cell*& cell(int row, int column) { return cells_[GetCellIndex(row, column)]; }
  const Cell* cell(int row, int column) const {
    return cells_[GetCellIndex(row, column)];
  }

  void SetSizes(int row_count, int column_count);
  void SetCellFormula(int row, int column, std::string_view formula);

  bool FindEmptyCell(int& row, int& column);

  void ClearCell(int row, int column);

  // GridModel
  virtual int GetRowCount() override;
  virtual void GetCell(ui::GridCell& data) override;
  virtual bool SetCellText(int row,
                           int column,
                           const std::wstring& text) override;

 private:
  int GetCellIndex(int row, int column) const {
    DCHECK(row >= 0 && row < row_count_);
    DCHECK(column >= 0 && column < column_count_);
    return row * column_count_ + column;
  }

  TimedDataService& timed_data_service_;
  DialogService& dialog_service_;

  int row_count_;
  int column_count_;
  std::vector<Cell*> cells_;

  views::FixedRowModel row_model_{*this};
  ui::ColumnHeaderModel column_model_;
};

class CellView : public Controller,
                 public ContentsModel,
                 private views::GridController,
                 private views::ContextMenuController {
 public:
  explicit CellView(const ControllerContext& context);
  virtual ~CellView();

  // Controller
  virtual views::View* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual ContentsModel* GetContentsModel() { return this; }

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;

 private:
  void ClearSelection();

  // views::GridController
  virtual bool OnGridDrawCell(views::GridView& sender,
                              gfx::Canvas* canvas,
                              int row,
                              int col,
                              const gfx::Rect& rect) override;
  virtual void OnGridSelectionChanged(views::GridView& sender) override;
  virtual bool OnKeyPressed(views::GridView& sender,
                            ui::KeyboardCode key_code) override;

  // views::ContextMenuController
  virtual void ShowContextMenuForView(views::View* source,
                                      const gfx::Point& point) override;

  CellModel model_;

  std::unique_ptr<views::GridView> grid_;
};
