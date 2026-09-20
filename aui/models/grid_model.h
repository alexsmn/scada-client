#pragma once

#include "aui/models/edit_data.h"
#include "aui/models/header_model.h"
#include "aui/models/table_column.h"

#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>
#include <functional>
#include <vector>

namespace scada::aui {

class GridRange;

struct GridModelIndex {
  int row = -1;
  int column = -1;

  bool is_valid() const { return row >= 0 && column >= 0; }
};

class GridModel {
 public:
  using ModelChangedCallback = std::function<void(GridModel& model)>;
  using RowRangeCallback =
      std::function<void(GridModel& model, int first, int count)>;
  using RangeChangedCallback =
      std::function<void(GridModel& model, const GridRange& range)>;

  GridModel();
  virtual ~GridModel();

  std::u16string GetCellText(int row, int column);

  virtual void GetCell(GridCell& cell) = 0;
  virtual std::u16string GetHint(int row, int column);

  virtual bool IsEditable(int row, int column);
  virtual bool SetCellText(int row, int column, const std::u16string& text);
  virtual EditData GetEditData(int row, int column);
  // Invoked when `EditData.editor_type == BUTTON` and the button is clicked.
  virtual void HandleEditButton(int row, int column);

  // Notifies after the row range has been changed wholesale.
  [[nodiscard]] boost::signals2::scoped_connection SubscribeModelChanged(
      const ModelChangedCallback& callback);
  // Row insertion and removal are announced in pairs: the "-ing" signal before
  // the rows exist (or stop existing) and the "-ed" signal after. A Qt adapter
  // needs both, because `QItemSelectionModel` early-returns on a change it was
  // not told was coming: told only afterwards, a selection keeps row numbers
  // that now name different items, and can sit past `rowCount()` while still
  // reading as valid. `TableModel` has carried the same pairs all along; this
  // is `GridModel` catching up.
  [[nodiscard]] boost::signals2::scoped_connection SubscribeRowsAdding(
      const RowRangeCallback& callback);
  [[nodiscard]] boost::signals2::scoped_connection SubscribeRowsAdded(
      const RowRangeCallback& callback);
  [[nodiscard]] boost::signals2::scoped_connection SubscribeRowsRemoving(
      const RowRangeCallback& callback);
  [[nodiscard]] boost::signals2::scoped_connection SubscribeRowsRemoved(
      const RowRangeCallback& callback);
  // Notifies after data in the specified range has been changed.
  [[nodiscard]] boost::signals2::scoped_connection SubscribeRangeChanged(
      const RangeChangedCallback& callback);

 protected:
  // Rows are announced through these guards and through nothing else -- the
  // bare notifiers below them are private on purpose. An adapter turns the
  // pair into `beginInsertRows`/`endInsertRows`, and Qt treats an unmatched
  // end as a programming error, so a model that could emit only the second
  // half would trade one defect for a louder one.
  struct ScopedRowsAdding {
    ScopedRowsAdding(GridModel& model, int first, int count)
        : model_{model}, first_{first}, count_{count} {
      model_.NotifyRowsAdding(first_, count_);
    }

    ~ScopedRowsAdding() { model_.NotifyRowsAdded(first_, count_); }

    GridModel& model_;
    int first_;
    int count_;
  };

  struct ScopedRowsRemoving {
    ScopedRowsRemoving(GridModel& model, int first, int count)
        : model_{model}, first_{first}, count_{count} {
      model_.NotifyRowsRemoving(first_, count_);
    }

    ~ScopedRowsRemoving() { model_.NotifyRowsRemoved(first_, count_); }

    GridModel& model_;
    int first_;
    int count_;
  };

  // A wholesale change, announced after the fact. Adapters answer it with a
  // model reset rather than a layout change.
  void NotifyModelChanged();

  void NotifyRangeChanged(const GridRange& range);
  void NotifyRowsChanged(int first, int count);

 private:
  void NotifyRowsAdding(int first, int count);
  void NotifyRowsAdded(int first, int count);
  void NotifyRowsRemoving(int first, int count);
  void NotifyRowsRemoved(int first, int count);

  boost::signals2::signal<void(GridModel&)> model_changed_signal_;
  boost::signals2::signal<void(GridModel&, int, int)> rows_adding_signal_;
  boost::signals2::signal<void(GridModel&, int, int)> rows_added_signal_;
  boost::signals2::signal<void(GridModel&, int, int)> rows_removing_signal_;
  boost::signals2::signal<void(GridModel&, int, int)> rows_removed_signal_;
  boost::signals2::signal<void(GridModel&, const GridRange&)>
      range_changed_signal_;
};

}  // namespace scada::aui
