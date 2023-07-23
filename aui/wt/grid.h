#pragma once

#include "base/values.h"
#include "aui/handlers.h"
#include "aui/models/grid_model.h"
#include "aui/models/grid_range.h"

#include <Wt/WPen.h>
#include <Wt/WTableView.h>

namespace aui {

class GridModel;
class GridModelAdapter;
class HeaderModel;

class Grid final : public Wt::WTableView {
 public:
  Grid(std::shared_ptr<GridModel> model,
       std::shared_ptr<HeaderModel> row_model,
       std::shared_ptr<HeaderModel> column_model);
  ~Grid();

  HeaderModel& row_model();
  HeaderModel& column_model();

  Wt::WWidget* CreateParentIfNecessary() { return this; }

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
    /*if (selectionModel()) {
      for (const auto& range : selectionModel()->selection()) {
        rows.reserve(rows.size() + range.height());
        for (int row = range.top(); row <= range.bottom(); ++row)
          rows.emplace_back(row);
      }
    }*/
    return rows;
  }

  auto GetSelectedColumns() const {
    std::vector<int> columns;
    /*if (selectionModel()) {
      for (const auto& range : selectionModel()->selection()) {
        columns.reserve(columns.size() + range.width());
        for (int column = range.left(); column <= range.right(); ++column)
          columns.emplace_back(column);
      }
    }*/
    return columns;
  }

  void SetSelectionChangeHandler(SelectionChangeHandler handler);

  void OpenEditor(const GridModelIndex& index);

  void RequestFocus();

  base::Value SaveState() const;
  void RestoreState(const base::Value& data);

 protected:
  // Wt::WTableView
  // virtual void mousePressEvent(Wt::WMouseEvent& event) override;
  //  void mouseReleaseEvent(Wt::WMouseEvent& e);
  //  void mouseMoveEvent(Wt::WMouseEvent& event);

 private:
  void UpdateSelectionRange();
  /*QRect GetRangeRect(const QItemSelectionRange& range) const;
  QRect GetExpandHandleRect(const QRect& selection_rect) const;
  QItemSelectionRange CalcExpandRange(const QModelIndex& index,
                                      QPoint pos) const;
  void SetExpandRange(const QItemSelectionRange& range);
  void Expand(const QItemSelectionRange& range,
              const QItemSelectionRange& expand_range);*/

  std::shared_ptr<GridModelAdapter> model_adapter_;

  ContextMenuHandler context_menu_handler_;

  const std::shared_ptr<GridModel> model_;

  // QItemSelectionRange selection_range_;

  bool expand_allowed_ = false;
  bool expanding_ = false;
  // QItemSelectionRange expand_range_;

  Wt::Signals::connection context_menu_connection_;
};

}  // namespace aui
