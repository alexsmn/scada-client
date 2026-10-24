#pragma once

#include "export/export_model.h"

#include <memory>
#include <string>
#include <vector>

class PrintService;

#if defined(UI_QT)
class QTextDocument;
#endif

// The lines a printout opens with, so a sheet of paper says what it is a
// record of: the view's title, then the period, the method and the time it
// was printed (backlog 875).
struct PrintHeader {
  std::u16string title;
  std::vector<std::u16string> lines;
};

void Print(PrintService& print_service,
           const ExportModel::TableExportData& table,
           const PrintHeader& header = {});

void Print(PrintService& print_service,
           const ExportModel::GridExportData& grid,
           const PrintHeader& header = {});

#if defined(UI_QT)
// The documents Print() sends to the printer. Exposed for tests.
std::unique_ptr<QTextDocument> MakePrintDocument(
    const ExportModel::TableExportData& table,
    const PrintHeader& header);
std::unique_ptr<QTextDocument> MakePrintDocument(
    const ExportModel::GridExportData& grid,
    const PrintHeader& header);
#endif
