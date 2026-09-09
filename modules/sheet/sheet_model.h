#pragma once

#include "aui/models/fixed_row_model.h"
#include "aui/models/grid_model.h"
#include "base/blinker.h"
#include "base/check.h"
#include "base/lifetime.h"
#include "modules/sheet/sheet_format.h"

#include <set>

class BlinkerManager;
class SheetCell;
class TimedDataService;
class WindowDefinition;

class SheetColumnModel : public scada::aui::ColumnHeaderModel {
 public:
  // aui::HeaderModel
  virtual std::u16string GetTitle(int index) const override;
};

struct SheetModelContext {
  TimedDataService& timed_data_service_;
  BlinkerManager& blinker_manager_;
};

class SheetModel : private SheetModelContext,
                   public scada::aui::GridModel,
                   private scada::aui::FixedRowModel::Delegate,
                   private Blinker {
 public:
  explicit SheetModel(SheetModelContext&& context);
  virtual ~SheetModel();

  void Load(const WindowDefinition& definition);
  void Save(WindowDefinition& definition);

  scada::aui::FixedRowModel& row_model() SCADA_LIFETIME_BOUND {
    return row_model_;
  }
  SheetColumnModel& column_model() SCADA_LIFETIME_BOUND {
    return column_model_;
  }

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

  void ClearRange(const scada::aui::GridRange& range);

  scada::aui::Color GetRangeColor(const scada::aui::GridRange& range) const;
  void SetRangeColor(const scada::aui::GridRange& range,
                     scada::aui::Color color);

  SheetFormatPool& formats() SCADA_LIFETIME_BOUND { return formats_; }

  TimedDataService& timed_data_service() { return timed_data_service_; }

  // aui::GridModel
  virtual int GetRowCount() override;
  virtual void GetCell(scada::aui::GridCell& cell) override;
  virtual bool SetCellText(int row,
                           int column,
                           const std::u16string& text) override;
  virtual bool IsEditable(int row, int column) override;

 private:
  // Blinker
  virtual void OnBlink(bool state);

  SheetFormatPool formats_;

  int row_count_ = 0;
  int column_count_ = 0;

  typedef std::set<SheetCell*> CellSet;
  // Declared before `cells_` so it is destroyed after them: ~SheetCell calls
  // SetBlinking(false), which erases the cell from this set. With the set
  // declared second it was already gone by then, and tearing down a sheet
  // holding one blinking cell segfaulted in std::set::erase.
  CellSet blinking_cells_;

  std::vector<std::unique_ptr<SheetCell>> cells_;

  bool editing_ = false;

  scada::aui::FixedRowModel row_model_{*this};
  SheetColumnModel column_model_;

  friend class SheetCell;
};

inline std::unique_ptr<SheetCell>& SheetModel::mutable_cell(int row, int col) {
  scada::base::Check(row >= 0 && row < row_count_);
  scada::base::Check(col >= 0 && col < column_count_);
  return cells_[row * column_count() + col];
}

inline const SheetCell* SheetModel::cell(int row, int col) const {
  scada::base::Check(row >= 0 && row < row_count_);
  scada::base::Check(col >= 0 && col < column_count_);
  return cells_[row * column_count() + col].get();
}
