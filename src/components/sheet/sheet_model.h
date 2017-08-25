#pragma once

#include <set>

#include "client/base/blinker.h"
#include "client/components/sheet/sheet_format.h"
#include "ui/base/models/grid_model.h"
#include "ui/base/models/fixed_row_model.h"

class SheetCell;
class TimedDataService;

class SheetColumnModel : public ui::ColumnHeaderModel {
 public:
  // ui::HeaderModel
  virtual base::string16 GetTitle(int index) override;
};

class SheetModel : public ui::GridModel,
                   private views::FixedRowModel::Delegate,
                   private Blinker {
 public:
  explicit SheetModel(TimedDataService& timed_data_service);
  virtual ~SheetModel();

  views::FixedRowModel& row_model() { return row_model_; }
  SheetColumnModel& column_model() { return column_model_; }

  int column_count() const { return column_count_; }
  void SetSizes(int nrow, int ncol);

  bool is_editing() const { return editing_; }
  void SetEditing(bool editing);

  // Get pointer to cell. May return NULL.
  SheetCell*& FindCell(int row, int column);
  const SheetCell* FindCell(int row, int column) const;
  // Returns existing cell or creates new one.
  SheetCell& GetCell(int row, int column);

  void ClearRange(const ui::GridRange& range);
  void ClearCell(int row, int col);

  void SetRangeColor(const ui::GridRange& range, SkColor color);

  SheetFormatPool& formats() { return formats_; }

  TimedDataService& timed_data_service() { return timed_data_service_; }
  
  // ui::GridModel
  virtual int GetRowCount() override;
  virtual void GetCell(ui::GridCell& cell) override;
  virtual bool SetCellText(int row, int column, const base::string16& text) override;

 protected:
  friend class SheetCell;

 private:
  // Blinker
	virtual void OnBlink(bool state);

  TimedDataService& timed_data_service_;

  int row_count_;
  int column_count_;
  std::vector<SheetCell*> cells_;

  SheetFormatPool formats_;

  typedef std::set<SheetCell*> CellSet;
  CellSet blinking_cells_;

  bool editing_;

  views::FixedRowModel row_model_;
  SheetColumnModel column_model_;
};

inline SheetCell*& SheetModel::FindCell(int row, int col) {
  assert(row >= 0 && row < row_count_);
  assert(col >= 0 && col < column_count_);
  return cells_[row * column_count() + col];
}

inline const SheetCell* SheetModel::FindCell(int row, int col) const {
  assert(row >= 0 && row < row_count_);
  assert(col >= 0 && col < column_count_);
  return cells_[row * column_count() + col];
}
