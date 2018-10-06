#pragma once

#include <memory>

#include "contents_model.h"
#include "controller.h"
#include "timed_data/timed_data_spec.h"
#include "ui/base/models/fixed_row_model.h"
#include "ui/base/models/grid_model.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/grid/grid_controller.h"

class CellView : public Controller,
                 public ContentsModel,
                 private ui::GridModel,
                 private views::FixedRowModel::Delegate,
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
  class Cell {
   public:
    Cell(CellView& view, int row, int column);

    rt::TimedDataSpec value_spec_;
    int row_;
    int column_;

   private:
    DISALLOW_COPY_AND_ASSIGN(Cell);
  };

  int GetCellIndex(int row, int column) const {
    DCHECK(row >= 0 && row < row_count_);
    DCHECK(column >= 0 && column < column_count_);
    return row * column_count_ + column;
  }

  Cell*& cell(int row, int column) { return cells_[GetCellIndex(row, column)]; }
  const Cell* cell(int row, int column) const {
    return cells_[GetCellIndex(row, column)];
  }

  void SetSizes(int row_count, int column_count);
  void SetCellFormula(int row, int column, base::StringPiece formula);

  bool FindEmptyCell(int& row, int& column);

  void ClearCell(int row, int column);
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

  // GridModel
  virtual int GetRowCount() override;
  virtual void GetCell(ui::GridCell& data) override;
  virtual bool SetCellText(int row, int column, const base::string16& text) override;

  // views::ContextMenuController
  virtual void ShowContextMenuForView(views::View* source,
                                      const gfx::Point& point) override;

  int row_count_;
  int column_count_;
  std::vector<Cell*> cells_;

  views::FixedRowModel row_model_;
  ui::ColumnHeaderModel column_model_;

  std::unique_ptr<views::GridView> grid_;
};
