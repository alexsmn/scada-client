#pragma once

#include "base/blinker.h"
#include "components/sheet/sheet_format.h"
#include "ui/base/models/fixed_row_model.h"
#include "ui/base/models/grid_model.h"
#include "window_definition.h"

#include <set>

class BlinkerManager;
class SheetCell;
class TimedDataService;

class SheetColumnModel : public ui::ColumnHeaderModel {
 public:
  // ui::HeaderModel
  virtual std::wstring GetTitle(int index) const override;
};

struct SheetModelContext {
  TimedDataService& timed_data_service_;
  BlinkerManager& blinker_manager_;
};

class SheetModel : private SheetModelContext,
                   public ui::GridModel,
                   private views::FixedRowModel::Delegate,
                   private Blinker {
 public:
  explicit SheetModel(SheetModelContext&& context);
  virtual ~SheetModel();

  void Load(const WindowDefinition& definition);
  void Save(WindowDefinition& definition);

  views::FixedRowModel& row_model() { return row_model_; }
  SheetColumnModel& column_model() { return column_model_; }

  int column_count() const { return column_count_; }
  int row_count() const { return row_count_; }
  void SetSizes(int nrow, int ncol);

  bool is_editing() const { return editing_; }
  void SetEditing(bool editing);

  // Get pointer to cell. May return NULL.
  std::unique_ptr<SheetCell>& mutable_cell(int row, int column);
  const SheetCell* cell(int row, int column) const;
  // Returns existing cell or creates new one.
  SheetCell& GetCell(int row, int column);

  void ClearRange(const ui::GridRange& range);
  void ClearCell(int row, int col);

  aui::Color GetRangeColor(const ui::GridRange& range) const;
  void SetRangeColor(const ui::GridRange& range, aui::Color color);

  SheetFormatPool& formats() { return formats_; }

  TimedDataService& timed_data_service() { return timed_data_service_; }

  // ui::GridModel
  virtual int GetRowCount() override;
  virtual void GetCell(ui::GridCell& cell) override;
  virtual bool SetCellText(int row,
                           int column,
                           const std::wstring& text) override;
  virtual bool IsEditable(int row, int column) override;

 private:
  // Blinker
  virtual void OnBlink(bool state);

  SheetFormatPool formats_;

  int row_count_ = 0;
  int column_count_ = 0;
  std::vector<std::unique_ptr<SheetCell>> cells_;

  typedef std::set<SheetCell*> CellSet;
  CellSet blinking_cells_;

  bool editing_ = false;

  views::FixedRowModel row_model_{*this};
  SheetColumnModel column_model_;

  friend class SheetCell;
};

inline std::unique_ptr<SheetCell>& SheetModel::mutable_cell(int row, int col) {
  assert(row >= 0 && row < row_count_);
  assert(col >= 0 && col < column_count_);
  return cells_[row * column_count() + col];
}

inline const SheetCell* SheetModel::cell(int row, int col) const {
  assert(row >= 0 && row < row_count_);
  assert(col >= 0 && col < column_count_);
  return cells_[row * column_count() + col].get();
}
