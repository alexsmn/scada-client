#pragma once

#include <vector>

namespace ui {
class GridModel;
class HeaderModel;
class TableModel;
struct TableColumn;
}  // namespace ui

class PrintService;

struct PrintTableContext {
  PrintService& print_service;
  ui::TableModel& model;
  const std::vector<ui::TableColumn>& columns;
};

void PrintTable(const PrintTableContext& context);

struct PrintGridContext {
  PrintService& print_service;
  ui::GridModel& model;
  const ui::HeaderModel& column_model;
  const ui::HeaderModel& row_model;
};

void PrintGrid(const PrintGridContext& context);
