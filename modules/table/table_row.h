#pragma once

#include "base/blinker.h"
#include "base/lifetime.h"
#include "modules/table/table_types.h"
#include "timed_data/timed_data_spec.h"

class TableModel;

class TableRow : private Blinker {
 public:
  TableRow(TableModel& model, int index);
  ~TableRow();

  TableRow(const TableRow&) = delete;
  TableRow& operator=(const TableRow&) = delete;

  int index() const { return index_; }
  void set_index(int index) { index_ = index; }

  const TimedDataSpec& timed_data() const SCADA_LIFETIME_BOUND {
    return timed_data_;
  }
  bool is_blinking() const { return is_blinking_; }

  std::string GetFormula() const;
  std::u16string GetTitle() const;
  std::u16string GetTooltip() const;

  void SetFormula(std::string formula, bool notify_update = true);

  void GetCellEx(TableCellEx& cell) const;

  void NotifyUpdate();

 private:
  void SetBlinking(bool blinking);

  // True once a reading has actually arrived for this row. False for a row
  // whose formula resolves to no node, or to a node the server does not have:
  // the Value cell is empty and the quality column reads "No data". The
  // timestamp cells follow it, so they never date a value that never came.
  bool HasDeliveredValue() const;

  void GetValueCell(TableCellEx& cell) const;
  void GetQualityCell(TableCellEx& cell) const;
  void GetEventCell(TableCellEx& cell) const;

  // Blinker events
  virtual void OnBlink(bool state) override;

  TableModel& model_;
  int index_;

  std::string formula_;
  TimedDataSpec timed_data_;
  bool is_blinking_ = false;
};
