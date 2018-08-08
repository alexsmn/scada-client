#pragma once

#include "controls/types.h"
#include "item_delegate.h"
#include "qt/grid_model_adapter.h"
#include "ui/base/models/grid_model.h"

#include <QTableView>

class Grid : public QTableView {
 public:
  Grid(ui::GridModel& model,
       ui::HeaderModel& row_model,
       ui::HeaderModel& column_model);
  ~Grid();

  QWidget* CreateParentIfNecessary() { return this; }

  void SetContextMenuHandler(ContextMenuHandler handler);

 private:
  GridModelAdapter model_adapter_;

  ContextMenuHandler context_menu_handler_;

  ui::GridModel& model_;

  ItemDelegate item_delegate_;
};
