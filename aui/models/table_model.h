#pragma once

#include "aui/models/table_column.h"

#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>
#include <functional>

namespace aui {

struct TableCell {
  int row = 0;
  int column_id = 0;
  std::u16string text;
  Color text_color = ColorCode::Transparent;
  Color cell_color = ColorCode::Transparent;
  int icon_index = -1;
};

class TableModel {
 public:
  using ModelChangedCallback = std::function<void()>;
  using ItemRangeCallback = std::function<void(int first, int count)>;

  TableModel();
  virtual ~TableModel();

  TableModel(const TableModel&) = delete;
  TableModel& operator=(const TableModel&) = delete;

  virtual int GetRowCount() = 0;

  std::u16string GetCellText(int row, int column_id);

  virtual void GetCell(TableCell& cell) = 0;
  virtual std::u16string GetTooltip(int row, int column_id);

  virtual bool SetCellText(int row, int column_id, const std::u16string& text);
  virtual bool IsEditable(int row, int column_id);

  virtual void Sort(int column_id, bool ascending);
  virtual int CompareCells(int row1, int row2, int column_id);

  // Notifies after the row range has been changed wholesale.
  [[nodiscard]] boost::signals2::scoped_connection SubscribeModelChanged(
      const ModelChangedCallback& callback);
  // Notifies after data in the specified range has been changed.
  [[nodiscard]] boost::signals2::scoped_connection SubscribeItemsChanged(
      const ItemRangeCallback& callback);
  [[nodiscard]] boost::signals2::scoped_connection SubscribeItemsAdding(
      const ItemRangeCallback& callback);
  [[nodiscard]] boost::signals2::scoped_connection SubscribeItemsAdded(
      const ItemRangeCallback& callback);
  [[nodiscard]] boost::signals2::scoped_connection SubscribeItemsRemoving(
      const ItemRangeCallback& callback);
  [[nodiscard]] boost::signals2::scoped_connection SubscribeItemsRemoved(
      const ItemRangeCallback& callback);

 protected:
  struct ScopedItemsAdding {
    ScopedItemsAdding(TableModel& model, int first, int count)
        : model_{model}, first_{first}, count_{count} {
      model_.NotifyItemsAdding(first_, count_);
    }

    ~ScopedItemsAdding() { model_.NotifyItemsAdded(first_, count_); }

    TableModel& model_;
    int first_;
    int count_;
  };

  struct ScopedItemsRemoving {
    ScopedItemsRemoving(TableModel& model, int first, int count)
        : model_{model}, first_{first}, count_{count} {
      model_.NotifyItemsRemoving(first_, count_);
    }

    ~ScopedItemsRemoving() { model_.NotifyItemsRemoved(first_, count_); }

    TableModel& model_;
    int first_;
    int count_;
  };

  void NotifyModelChanged();
  void NotifyItemsChanged(int first, int count);
  void NotifyItemsAdding(int first, int count);
  void NotifyItemsAdded(int first, int count);
  void NotifyItemsRemoving(int first, int count);
  void NotifyItemsRemoved(int first, int count);

 private:
  boost::signals2::signal<void()> model_changed_signal_;
  boost::signals2::signal<void(int, int)> items_changed_signal_;
  boost::signals2::signal<void(int, int)> items_adding_signal_;
  boost::signals2::signal<void(int, int)> items_added_signal_;
  boost::signals2::signal<void(int, int)> items_removing_signal_;
  boost::signals2::signal<void(int, int)> items_removed_signal_;
};

}  // namespace aui
