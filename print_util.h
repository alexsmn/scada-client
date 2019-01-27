#pragma once

#include "export_model.h"

class PrintService;

void Print(PrintService& print_service,
           const ExportModel::TableExportData& table);

void Print(PrintService& print_service,
           const ExportModel::GridExportData& grid);
