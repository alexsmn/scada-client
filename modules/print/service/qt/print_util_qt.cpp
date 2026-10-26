#include "print/service/print_util.h"

#include "aui/models/grid_model.h"
#include "aui/models/table_model.h"
#include "export/image_export_model.h"
#include "print/service/print_service.h"

#include <algorithm>

#include <QFontMetrics>
#include <QPainter>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextTable>

namespace {

// TODO: Can be a duplicate with Table.
Qt::Alignment MakeQtAlignment(scada::aui::TableColumn::Alignment alignment) {
  Qt::Alignment result;
  switch (alignment) {
    case scada::aui::TableColumn::LEFT:
      result |= Qt::AlignLeft;
      break;
    case scada::aui::TableColumn::RIGHT:
      result |= Qt::AlignRight;
      break;
    case scada::aui::TableColumn::CENTER:
      result |= Qt::AlignCenter;
      break;
  }
  return result;
}

}  // namespace

class TableDocumentBuilder {
 public:
  TableDocumentBuilder(const PrintHeader& header,
                       int row_count,
                       int column_count);

  void SetColumn(int column,
                 const std::u16string& title,
                 scada::aui::TableColumn::Alignment alignment);

  void SetCell(int row, int column, const std::u16string& text);

  std::unique_ptr<QTextDocument> Build();

 private:
  void InsertHeader(const PrintHeader& header);

  const int row_count_;
  const int column_count_;

  QTextCharFormat row_format_;
  QTextCharFormat alternate_row_format_;
  QTextCharFormat header_format_;
  std::vector<QTextBlockFormat> column_formats_;

  std::unique_ptr<QTextDocument> document_ = std::make_unique<QTextDocument>();
  QTextCursor cursor_{document_.get()};
  QTextTable* table_ = nullptr;
};

TableDocumentBuilder::TableDocumentBuilder(const PrintHeader& header,
                                           int row_count,
                                           int column_count)
    : row_count_{row_count},
      column_count_{column_count},
      column_formats_(column_count) {
  // document.setDocumentMargin(10);

  cursor_.movePosition(QTextCursor::Start);

  InsertHeader(header);

  QTextTableFormat table_format;
  table_format.setBorderStyle(QTextFrameFormat::BorderStyle_None);
  table_format.setHeaderRowCount(1);
  table_format.setCellSpacing(5);
  table_ = cursor_.insertTable(row_count_ + 1, column_count_, table_format);

  // TODO: Setup |row_format_|.

  alternate_row_format_ = row_format_;
  // alternate_row_format.setBackground(Qt::lightGray);

  header_format_ = row_format_;
  header_format_.setFontWeight(QFont::Bold);
}

void TableDocumentBuilder::InsertHeader(const PrintHeader& header) {
  // Page numbers need nothing here: QTextDocument::print paginates an
  // unpaginated document itself and numbers each page at the bottom.
  if (!header.title.empty()) {
    QTextCharFormat title_format;
    title_format.setFontWeight(QFont::Bold);
    title_format.setFontPointSize(14);
    cursor_.insertText(QString::fromStdU16String(header.title), title_format);
    cursor_.insertBlock();
  }

  for (const std::u16string& line : header.lines) {
    cursor_.insertText(QString::fromStdU16String(line), QTextCharFormat{});
    cursor_.insertBlock();
  }
}

void TableDocumentBuilder::SetColumn(
    int column,
    const std::u16string& title,
    scada::aui::TableColumn::Alignment alignment) {
  column_formats_[column].setAlignment(MakeQtAlignment(alignment));

  auto cell = table_->cellAt(0, column);
  auto cursor = cell.firstCursorPosition();
  cursor.setBlockFormat(column_formats_[column]);
  cursor.insertText(QString::fromStdU16String(title), header_format_);
}

void TableDocumentBuilder::SetCell(int row,
                                   int column,
                                   const std::u16string& text) {
  auto cell = table_->cellAt(row + 1, column);
  cell.setFormat(row % 2 == 0 ? row_format_ : alternate_row_format_);
  auto cursor = cell.firstCursorPosition();
  cursor.setBlockFormat(column_formats_[column]);
  cursor.insertText(QString::fromStdU16String(text));
}

std::unique_ptr<QTextDocument> TableDocumentBuilder::Build() {
  cursor_.movePosition(QTextCursor::End);
  cursor_.insertBlock();

  return std::move(document_);
}

std::unique_ptr<QTextDocument> MakePrintDocument(
    const ExportModel::TableExportData& table,
    const PrintHeader& header) {
  // A printout is a record like the spreadsheet exports, so it prints one row
  // per event where the view groups them.
  scada::aui::TableModel& model = table.ModelFor(/*expanded=*/true);
  const auto row_range = table.GetRowRange(/*expanded=*/true);
  const int column_count = static_cast<int>(table.columns.size());

  TableDocumentBuilder builder{header, row_range.count, column_count};

  for (int i = 0; i < column_count; ++i)
    builder.SetColumn(i, table.columns[i].title, table.columns[i].alignment);

  for (int row = 0; row < row_range.count; ++row) {
    for (int column = 0; column < column_count; ++column) {
      auto text =
          model.GetCellText(row_range.first + row, table.columns[column].id);
      builder.SetCell(row, column, text);
    }
  }

  return builder.Build();
}

std::unique_ptr<QTextDocument> MakePrintDocument(
    const ExportModel::GridExportData& grid,
    const PrintHeader& header) {
  const int row_count = grid.rows.GetCount();
  const int column_count = grid.columns.GetCount();

  // Add a row header, titled as the export titles it (the summary's
  // "Time (interval start)"); it printed with no title at all (backlog 875).
  TableDocumentBuilder builder{header, row_count, 1 + column_count};

  builder.SetColumn(0, grid.row_title_column.title,
                    grid.row_title_column.alignment);
  for (int i = 0; i < column_count; ++i) {
    builder.SetColumn(1 + i, grid.columns.GetTitle(i),
                      grid.columns.GetAlignment(i));
  }

  for (int row = 0; row < row_count; ++row) {
    builder.SetCell(row, 0, grid.rows.GetTitle(row));
    for (int column = 0; column < column_count; ++column)
      builder.SetCell(row, 1 + column, grid.model.GetCellText(row, column));
  }

  return builder.Build();
}

void Print(PrintService& print_service,
           const ExportModel::TableExportData& table,
           const PrintHeader& header) {
  MakePrintDocument(table, header)->print(&print_service.printer);
}

void Print(PrintService& print_service,
           const ExportModel::GridExportData& grid,
           const PrintHeader& header) {
  MakePrintDocument(grid, header)->print(&print_service.printer);
}

void PaintImagePage(QPainter& painter,
                    const QRect& page,
                    ImageExportModel& image,
                    const PrintHeader& header) {
  painter.save();

  // The header, as a table printout opens: a bold title, then one line each.
  int top = page.top();
  if (!header.title.empty()) {
    QFont title_font = painter.font();
    title_font.setBold(true);
    title_font.setPointSizeF(title_font.pointSizeF() * 1.4);
    painter.setFont(title_font);
    const int height = QFontMetrics{title_font, painter.device()}.lineSpacing();
    painter.drawText(QRect{page.left(), top, page.width(), height},
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QString::fromStdU16String(header.title));
    top += height;
  }
  painter.setFont(QFont{});
  const int line_height = QFontMetrics{QFont{}, painter.device()}.lineSpacing();
  for (const std::u16string& line : header.lines) {
    painter.drawText(QRect{page.left(), top, page.width(), line_height},
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QString::fromStdU16String(line));
    top += line_height;
  }
  top += line_height / 2;

  // The picture, scaled to the space left and centred horizontally.
  const QSize size = image.GetImageSize();
  const QRect area{page.left(), top, page.width(), page.bottom() - top + 1};
  if (!size.isEmpty() && area.width() > 0 && area.height() > 0) {
    const qreal scale = std::min(qreal(area.width()) / size.width(),
                                 qreal(area.height()) / size.height());
    const qreal left = area.left() + (area.width() - size.width() * scale) / 2;
    painter.translate(left, area.top());
    painter.scale(scale, scale);
    painter.setClipRect(QRect{QPoint{}, size});
    image.RenderImage(painter);
  }

  painter.restore();
}

void Print(PrintService& print_service,
           ImageExportModel& image,
           const PrintHeader& header) {
  QPrinter& printer = print_service.printer;
  QPainter painter;
  if (!painter.begin(&printer))
    return;
  // Without fullPage the painter's origin is already the printable area's
  // top-left corner, so the page is that area's size at the origin.
  const QRect page{QPoint{}, printer.pageLayout()
                                 .paintRectPixels(printer.resolution())
                                 .size()};
  PaintImagePage(painter, page, image, header);
  painter.end();
}
