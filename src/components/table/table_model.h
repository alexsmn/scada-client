#pragma once

#include <functional>
#include <set>

#include "core/configuration_types.h"
#include "base/blinker.h"
#include "ui/base/models/table_model.h"

namespace events {
class EventManager;
}

class Profile;
class TableRow;
class TimedDataService;

class TableModel : public ui::TableModel,
                   private Blinker {
 public:
  enum ColumnType { COLUMN_TITLE,
                    COLUMN_VALUE,
                    COLUMN_CHANGE_TIME,
                    COLUMN_UPDATE_TIME,
                    COLUMN_EVENT };

  struct CellEx : public ui::TableCell {
    int image_index;
  };

  TableModel(TimedDataService& timed_data_service, events::EventManager& event_manager, Profile& profile);
  virtual ~TableModel();

  TimedDataService& timed_data_service() { return timed_data_service_; }

  int row_count() const { return static_cast<int>(rows_.size()); }
  TableRow* GetRow(int index);
  const TableRow* GetRow(int index) const;

  void GetCellEx(CellEx& cell);

  bool DeleteRows(int start, int count);
  void Clear();
  int MoveRow(int row, bool up);
  bool SetFormula(int row, const std::string& formula);

  int FindItem(const scada::NodeId& trid) const;

  void Sort(unsigned command_id);

  // ui::TableModel
  virtual int GetRowCount() override;
  virtual void GetCell(ui::TableCell& cell) override;
  virtual base::string16 GetTooltip(int row, int column_id) override;

  std::function<void(const scada::NodeId& item_id, bool added)> item_changed_;

 private:
  friend class TableRow;

  class RowsComparer {
   public:
    explicit RowsComparer(unsigned command_id) : command_id_(command_id) { }
    
    bool operator()(const TableRow* left, const TableRow* right) const;

   private:
    unsigned command_id_;
  };
  
  // Blinker events
  virtual void OnBlink(bool state) override;

  TimedDataService& timed_data_service_;
  events::EventManager& event_manager_;
  Profile& profile_;

  typedef std::vector<TableRow*> Rows;
  Rows rows_;

  typedef std::set<TableRow*> RowSet;
  RowSet blinking_rows_;
};