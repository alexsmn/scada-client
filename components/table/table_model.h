#pragma once

#include "components/table/table_types.h"
#include "core/configuration_types.h"

#include <functional>

class BlinkerManager;
class DialogService;
class NodeEventProvider;
class Profile;
class TableRow;
class TimedDataService;

struct TableModelContext {
  TimedDataService& timed_data_service_;
  NodeEventProvider& node_event_provider_;
  const Profile& profile_;
  DialogService& dialog_service_;
  BlinkerManager& blinker_manager_;
};

class TableModel : private TableModelContext, public ui::TableModel {
 public:
  enum ColumnId : int {
    COLUMN_TITLE,
    COLUMN_VALUE,
    COLUMN_CHANGE_TIME,
    COLUMN_SOURCE_TIMESTAMP,
    COLUMN_SERVER_TIMESTAMP,
    COLUMN_EVENT,

    COLUMN_FIRST = COLUMN_TITLE,
    COLUMN_LAST = COLUMN_EVENT,
  };

  explicit TableModel(TableModelContext&& context);
  virtual ~TableModel();

  TimedDataService& timed_data_service() { return timed_data_service_; }

  int row_count() const { return static_cast<int>(rows_.size()); }
  TableRow* GetRow(int index);
  const TableRow* GetRow(int index) const;

  void GetCellEx(TableCellEx& cell) const;

  bool DeleteRows(int start, int count);
  void Clear();
  int MoveRow(int row, bool up);
  bool SetFormula(int row, std::string formula);

  int FindItem(const scada::NodeId& trid) const;

  void Sort(unsigned command_id);

  // ui::TableModel
  virtual int GetRowCount() override;
  virtual void GetCell(ui::TableCell& cell) override;
  virtual std::wstring GetTooltip(int row, int column_id) override;
  virtual bool SetCellText(int row,
                           int column_id,
                           const std::wstring& text) override;
  virtual bool IsEditable(int row, int column) override;

  std::function<void(const scada::NodeId& item_id, bool added)> item_changed_;

 private:
  friend class TableRow;

  void OnRowNodeChanged(const scada::NodeId& old_node_id, const scada::NodeId& new_node_id);

  typedef std::vector<std::unique_ptr<TableRow>> Rows;
  Rows rows_;
};
