#pragma once

#include "export_model.h"

#include <filesystem>

class ExcelSheetModel;

void ExportToCsv(ExportModel::TableExportData& table,
                 const std::filesystem::path& path);
void ExportToCsv(ExportModel::GridExportData& grid,
                 const std::filesystem::path& path);

void ExportToExcel(ExportModel::TableExportData& table, ExcelSheetModel& sheet);
void ExportToExcel(ExportModel::GridExportData& grid, ExcelSheetModel& sheet);
