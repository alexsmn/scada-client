#pragma once

#include "base/blinker.h"
#include "core/configuration_types.h"
#include "ui/base/models/table_model.h"

#include <functional>
#include <set>

class BlinkerManager;
class DialogService;
class EventFetcher;
class Executor;
class Profile;
class TableRow;
class TimedDataService;

struct TableModelContext {
  const std::shared_ptr<Executor> executor_;
  TimedDataService& timed_data_service_;
  EventFetcher& event_fetcher_;
  Profile& profile_;
  DialogService& dialog_service_;
  BlinkerManager& blinker_manager_;
};

class TableModel : private TableModelContext,
                   public ui::TableModel,
                   private Blinker {
 public:
  enum ColumnType {
    COLUMN_TITLE,
    COLUMN_VALUE,
    COLUMN_CHANGE_TIME,
    COLUMN_UPDATE_TIME,
    COLUMN_EVENT
  };

  struct CellEx : public ui::TableCell {
    int image_index;
  };

  explicit TableModel(TableModelContext&& context);
  virtual ~TableModel();

  TimedDataService& timed_data_service() { return timed_data_service_; }

  int row_count() const { return static_cast<int>(rows_.size()); }
  TableRow* GetRow(int index);
  const TableRow* GetRow(int index) const;

  void GetCellEx(CellEx& cell);

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

  class RowsComparer {
   public:
    explicit RowsComparer(unsigned command_id) : command_id_(command_id) {}

    bool operator()(const TableRow* left, const TableRow* right) const;

   private:
    unsigned command_id_;
  };

  // Blinker events
  virtual void OnBlink(bool state) override;

  typedef std::vector<TableRow*> Rows;
  Rows rows_;

  typedef std::set<TableRow*> RowSet;
  RowSet blinking_rows_;
};
