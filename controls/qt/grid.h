#pragma once

#include "base/values.h"
#include "controls/handlers.h"
#include "controls/models/grid_range.h"
#include "controls/qt/grid_model_adapter.h"
#include "controls/qt/item_delegate.h"

#include <QPen>
#include <QTableView>

namespace aui {

class GridModel;
class HeaderModel;

class Grid final : public QTableView {
 public:
  Grid(std::shared_ptr<GridModel> model,
       std::shared_ptr<HeaderModel> row_model,
       std::shared_ptr<HeaderModel> column_model);
  ~Grid();

  HeaderModel& row_model() { return model_adapter_.row_model(); }
  HeaderModel& column_model() { return model_adapter_.column_model(); }

  QWidget* CreateParentIfNecessary() { return this; }

  void SetExpandAllowed(bool allowed);

  void SetColumnHeaderVisible(bool visible);
  void SetColumnHeaderHeight(int height);
  void SetRowHeaderVisible(bool visible);
  void SetRowHeaderWidth(int width);

  void SetContextMenuHandler(ContextMenuHandler handler);

  GridModelIndex GetCurrentIndex() const;

  GridRange GetSelectionRange() const;

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

  void OpenEditor(const GridModelIndex& index);

  base::Value SaveState() const;
  void RestoreState(const base::Value& data);

  void RequestFocus();

  void CopyToClipboard();

 protected:
  // QTableView
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  virtual void paintEvent(QPaintEvent* e) override;
  virtual void selectionChanged(const QItemSelection& selected,
                                const QItemSelection& deselected) override;
  virtual void keyPressEvent(QKeyEvent* event) override;

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

  const std::shared_ptr<GridModel> model_;

  ItemDelegate item_delegate_;

  QItemSelectionRange selection_range_;

  bool expand_allowed_ = false;
  bool expanding_ = false;
  QItemSelectionRange expand_range_;
};

}  // namespace aui
