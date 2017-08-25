#pragma once

#include "ui/views/controls/grid/grid_view.h"

class Grid : public views::GridView {
 public:
  Grid(ui::GridModel& model, ui::HeaderModel& row_model, ui::HeaderModel& column_model) {
    SetModel(&model);
    SetRowModel(&row_model);
    SetColumnModel(&column_model);
  }
};
