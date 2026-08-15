#pragma once

#include "base/any_executor.h"
#include "common/node_state.h"
#include "modules/table/table_types.h"

#include <functional>

class BlinkerManager;
class DialogService;
class NodeEventProvider;
class Profile;
class TableRow;
class TimedDataService;

struct TableModelContext {
  // `SetCellText` reports an invalid formula through a message box, which is
  // a lazy awaitable and needs an executor to be spawned on. See
  // `aui/show_message_box.h`.
  const AnyExecutor executor_;
  TimedDataService& timed_data_service_;
  NodeEventProvider& node_event_provider_;
  const Profile& profile_;
  DialogService& dialog_service_;
  BlinkerManager& blinker_manager_;
};

class TableModel : private TableModelContext, public scada::aui::TableModel {
 public:
  enum ColumnId : int {
    COLUMN_TITLE,
    COLUMN_VALUE,
    COLUMN_CHANGE_TIME,
    COLUMN_SOURCE_TIMESTAMP,
    COLUMN_SERVER_TIMESTAMP,
    COLUMN_EVENT,
    // Reshell-only good/uncertain/bad quality mark. Appended (not inserted) so
    // the existing column ids in saved window state stay stable; the column is
    // added to the view only under the opt-in token theme.
    COLUMN_QUALITY,
    // Reshell-only per-row mini-trend, painted by the sparkline delegate from
    // the row's trailing history window (no cell text). Appended for the same
    // saved-state stability reason.
    COLUMN_SPARKLINE,
    // What the row is bound to: its NodeId, or the expression for a computed
    // row. This is what tells an engineer where a value comes from — the job
    // a present/absent row icon used to do silently and without a label
    // (docs/product/ui-mockups/screens/table-watch.html shows it as its own
    // column). Appended for the same saved-state stability reason.
    COLUMN_SOURCE,

    COLUMN_FIRST = COLUMN_TITLE,
    COLUMN_LAST = COLUMN_SOURCE,
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
  virtual void GetCell(scada::aui::TableCell& cell) override;
  virtual std::u16string GetTooltip(int row, int column_id) override;
  virtual bool SetCellText(int row,
                           int column_id,
                           const std::u16string& text) override;
  virtual bool IsEditable(int row, int column) override;

  std::function<void(const scada::NodeId& item_id, bool added)> item_changed_;

 private:
  friend class TableRow;

  void OnRowNodeChanged(const scada::NodeId& old_node_id,
                        const scada::NodeId& new_node_id);

  typedef std::vector<std::unique_ptr<TableRow>> Rows;
  Rows rows_;
};
