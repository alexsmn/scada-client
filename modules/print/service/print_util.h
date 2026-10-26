#pragma once

#include "export/export_model.h"

#include <memory>
#include <string>
#include <vector>

class ImageExportModel;
class PrintService;

#if defined(UI_QT)
class QPainter;
class QRect;
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

// Prints a picture view (the graph) on one page: the header, then the picture
// scaled to fit the rest of the page with its aspect ratio kept (backlog 874).
void Print(PrintService& print_service,
           ImageExportModel& image,
           const PrintHeader& header = {});

#if defined(UI_QT)
// Paints what Print() puts on a page of |page| size. Exposed for tests.
void PaintImagePage(QPainter& painter,
                    const QRect& page,
                    ImageExportModel& image,
                    const PrintHeader& header);

// The documents Print() sends to the printer. Exposed for tests.
std::unique_ptr<QTextDocument> MakePrintDocument(
    const ExportModel::TableExportData& table,
    const PrintHeader& header);
std::unique_ptr<QTextDocument> MakePrintDocument(
    const ExportModel::GridExportData& grid,
    const PrintHeader& header);
#endif
