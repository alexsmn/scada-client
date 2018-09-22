#pragma once

#include <vector>

namespace ui {
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
