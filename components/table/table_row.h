#pragma once

#include "components/table/table_types.h"
#include "timed_data/timed_data_spec.h"

class TableModel;

class TableRow {
 public:
  TableRow(TableModel& model, int index);
  ~TableRow();

  int index() const { return index_; }
  void set_index(int index) { index_ = index; }

  const TimedDataSpec& timed_data() const { return timed_data_; }
  bool is_blinking() const { return is_blinking_; }

  std::string GetFormula() const;
  std::wstring GetTitle() const;

  void SetFormula(std::string formula);

  void GetCellEx(TableCellEx& cell) const;

  void NotifyUpdate();

 private:
  void SetBlinking(bool blinking);

  void GetValueCell(TableCellEx& cell) const;
  void GetEventCell(TableCellEx& cell) const;

  TableModel& model_;
  int index_;

  std::string formula_;
  TimedDataSpec timed_data_;
  bool is_blinking_ = false;

  DISALLOW_COPY_AND_ASSIGN(TableRow);
};
