#include "print_util.h"

#include "services/print_service.h"
#include "ui/base/models/table_model.h"

#include <QTextCursor>
#include <QTextDocument>
#include <QTextTable>

namespace {

Qt::Alignment MakeQtAlignment(ui::TableColumn::Alignment alignment) {
  Qt::Alignment result;
  switch (alignment) {
    case ui::TableColumn::LEFT:
      result |= Qt::AlignLeft;
      break;
    case ui::TableColumn::RIGHT:
      result |= Qt::AlignRight;
      break;
    case ui::TableColumn::CENTER:
      result |= Qt::AlignCenter;
      break;
  }
  return result;
}

}  // namespace

void PrintTable(const PrintTableContext& context) {
  const int row_count = context.model.GetRowCount();

  QTextTableFormat table_format;
  table_format.setBorderStyle(QTextFrameFormat::BorderStyle_None);
  table_format.setHeaderRowCount(1);
  table_format.setCellSpacing(5);

  QTextCharFormat row_format;

  QTextCharFormat header_format = row_format;
  header_format.setFontWeight(QFont::Bold);

  QTextCharFormat alternate_row_format = row_format;
  // alternate_row_format.setBackground(Qt::lightGray);

  std::vector<QTextBlockFormat> column_formats(context.columns.size());
  for (int i = 0; i < static_cast<int>(column_formats.size()); ++i) {
    column_formats[i].setAlignment(
        MakeQtAlignment(context.columns[i].alignment));
  }

  QTextDocument document;
  // document.setDocumentMargin(10);

  QTextCursor cursor{&document};
  cursor.movePosition(QTextCursor::Start);

  QTextTable* table =
      cursor.insertTable(row_count + 1, context.columns.size(), table_format);
  for (int column_index = 0;
       column_index < static_cast<int>(context.columns.size());
       ++column_index) {
    auto cell = table->cellAt(0, column_index);
    auto cursor = cell.firstCursorPosition();
    cursor.setBlockFormat(column_formats[column_index]);
    cursor.insertText(
        QString::fromStdWString(context.columns[column_index].title),
        header_format);
  }

  for (int row_index = 0; row_index < row_count; ++row_index) {
    auto& cellFormat = row_index % 2 == 0 ? row_format : alternate_row_format;

    for (int column_index = 0;
         column_index < static_cast<int>(context.columns.size());
         ++column_index) {
      auto cell = table->cellAt(row_index + 1, column_index);
      cell.setFormat(cellFormat);
      auto cursor = cell.firstCursorPosition();
      cursor.setBlockFormat(column_formats[column_index]);
      cursor.insertText(QString::fromStdWString(context.model.GetCellText(
          row_index, context.columns[column_index].id)));
    }
  }

  cursor.movePosition(QTextCursor::End);
  cursor.insertBlock();

  // Print to PDF
  document.print(&context.print_service.printer);
}
