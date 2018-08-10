#pragma once

#include "controls/types.h"
#include "item_delegate.h"
#include "qt/grid_model_adapter.h"
#include "ui/base/models/grid_model.h"

#include <QPen>
#include <QTableView>

class Grid : public QTableView {
 public:
  Grid(ui::GridModel& model,
       ui::HeaderModel& row_model,
       ui::HeaderModel& column_model);
  ~Grid();

  QWidget* CreateParentIfNecessary() { return this; }

  void SetExpandAllowed(bool allowed);

  void SetContextMenuHandler(ContextMenuHandler handler);

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
