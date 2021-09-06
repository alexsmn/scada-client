#pragma once

#include "base/values.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/grid/grid_controller.h"
#include "ui/views/controls/grid/grid_view.h"

#include <boost/range/irange.hpp>

class Grid final : private views::ContextMenuController,
                   private views::GridController,
                   public views::GridView {
 public:
  Grid(ui::GridModel& model,
       ui::HeaderModel& row_model,
       ui::HeaderModel& column_model) {
    SetModel(&model);
    SetRowModel(&row_model);
    SetColumnModel(&column_model);
    set_controller(this);
  }

  void SetContextMenuHandler(ContextMenuHandler handler) {
    context_menu_handler_ = std::move(handler);
    set_context_menu_controller(this);
  }

  ui::GridModelIndex GetCurrentIndex() const {
    return {selection().row(), selection().column()};
  }

  auto GetSelectedRows() const {
    return boost::irange(selection().row(),
                         selection().row() + selection().row_count());
  }

  auto GetSelectedColumns() const {
    return boost::irange(selection().column(),
                         selection().column() + selection().column_count());
  }

  void SetSelectionChangeHandler(SelectionChangeHandler handler) {
    selection_changed_handler_ = std::move(handler);
  }

  void OpenEditor(const ui::GridModelIndex& index) {
    views::GridView::OpenEditor(index.row, index.column);
  }

  base::Value SaveState() const { return {}; }
  void RestoreState(const base::Value& data) {}

 private:
  // views::GridController
  virtual void OnGridSelectionChanged(GridView& sender) override {
    if (selection_changed_handler_)
      selection_changed_handler_();
  }

  // views::ContextMenuController
  virtual void ShowContextMenuForView(views::View* source,
                                      const gfx::Point& point) override {
    if (context_menu_handler_)
      context_menu_handler_(point);
  }

  SelectionChangedHandler selection_changed_handler_;
  ContextMenuHandler context_menu_handler_;
};
