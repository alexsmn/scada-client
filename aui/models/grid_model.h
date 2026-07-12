#pragma once

#include "aui/aui_ns_compat.h"

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
  [[nodiscard]] boost::signals2::scoped_connection SubscribeRowsAdded(
      const RowRangeCallback& callback);
  [[nodiscard]] boost::signals2::scoped_connection SubscribeRowsRemoved(
      const RowRangeCallback& callback);
  // Notifies after data in the specified range has been changed.
  [[nodiscard]] boost::signals2::scoped_connection SubscribeRangeChanged(
      const RangeChangedCallback& callback);

 protected:
  void NotifyModelChanged();
  void NotifyRowsAdded(int first, int count);
  void NotifyRowsRemoved(int first, int count);

  void NotifyRangeChanged(const GridRange& range);
  void NotifyRowsChanged(int first, int count);

 private:
  boost::signals2::signal<void(GridModel&)> model_changed_signal_;
  boost::signals2::signal<void(GridModel&, int, int)> rows_added_signal_;
  boost::signals2::signal<void(GridModel&, int, int)> rows_removed_signal_;
  boost::signals2::signal<void(GridModel&, const GridRange&)>
      range_changed_signal_;
};

}  // namespace aui
