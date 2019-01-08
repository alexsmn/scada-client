#pragma once

#include "controls/types.h"
#include "item_delegate.h"
#include "qt/grid_model_adapter.h"
#include "ui/base/models/grid_model.h"

#include <QPen>
#include <QTableView>

class Grid final : public QTableView {
 public:
  Grid(ui::GridModel& model,
       ui::HeaderModel& row_model,
       ui::HeaderModel& column_model);
  ~Grid();

  ui::HeaderModel& row_model() { return model_adapter_.row_model(); }
  ui::HeaderModel& column_model() { return model_adapter_.column_model(); }

  QWidget* CreateParentIfNecessary() { return this; }

  void SetExpandAllowed(bool allowed);

  void SetColumnHeaderVisible(bool visible);
  void SetColumnHeaderHeight(int height);
  void SetRowHeaderVisible(bool visible);
  void SetRowHeaderWidth(int width);

  void SetContextMenuHandler(ContextMenuHandler handler);

  ui::GridModelIndex GetCurrentIndex() const;

  auto GetSelectedRows() const {
    std::vector<int> rows;
    if (selectionModel()) {
      for (const QItemSelectionRange& range : selectionModel()->selection()) {
        rows.reserve(rows.size() + range.height());
        for (int row = range.top(); row <= range.bottom(); ++row)
          rows.emplace_back(row);
      }
    }
    return rows;
  }

  auto GetSelectedColumns() const {
    std::vector<int> columns;
    if (selectionModel()) {
      for (const QItemSelectionRange& range : selectionModel()->selection()) {
        columns.reserve(columns.size() + range.width());
        for (int column = range.left(); column <= range.right(); ++column)
          columns.emplace_back(column);
      }
    }
    return columns;
  }

  void SetSelectionChangeHandler(SelectionChangeHandler handler);

  void OpenEditor(const ui::GridModelIndex& index);

  base::Value SaveState() const;
  void RestoreState(const base::Value& data);

 protected:
  // QTableView
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  virtual void paintEvent(QPaintEvent* e) override;
  virtual void selectionChanged(const QItemSelection& selected,
                                const QItemSelection& deselected) override;

 private:
  void UpdateSelectionRange();
  QRect GetRangeRect(const QItemSelectionRange& range) const;
  QRect GetExpandHandleRect(const QRect& selection_rect) const;
  QItemSelectionRange CalcExpandRange(const QModelIndex& index,
                                      QPoint pos) const;
  void SetExpandRange(const QItemSelectionRange& range);
  void Expand(const QItemSelectionRange& range,
              const QItemSelectionRange& expand_range);

  GridModelAdapter model_adapter_;

  ContextMenuHandler context_menu_handler_;

  ui::GridModel& model_;

  ItemDelegate item_delegate_;

  QItemSelectionRange selection_range_;

  bool expand_allowed_ = false;
  bool expanding_ = false;
  QItemSelectionRange expand_range_;
};
