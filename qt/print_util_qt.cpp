#include "print_util.h"

#include "services/print_service.h"
#include "ui/base/models/grid_model.h"
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

class TableDocumentBuilder {
 public:
  explicit TableDocumentBuilder(int row_count, int column_count);

  void SetColumn(int column,
                 const base::string16& title,
                 ui::TableColumn::Alignment alignment);

  void SetCell(int row, int column, const base::string16& text);

  QTextDocument& Build();

 private:
  const int row_count_;
  const int column_count_;

  QTextCharFormat row_format_;
  QTextCharFormat alternate_row_format_;
  QTextCharFormat header_format_;
  std::vector<QTextBlockFormat> column_formats_;

  QTextDocument document_;
  QTextCursor cursor_{&document_};
  QTextTable* table_ = nullptr;
};

TableDocumentBuilder::TableDocumentBuilder(int row_count, int column_count)
    : row_count_{row_count},
      column_count_{column_count},
      column_formats_(column_count) {
  // document.setDocumentMargin(10);

  cursor_.movePosition(QTextCursor::Start);

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

void TableDocumentBuilder::SetColumn(int column,
                                     const base::string16& title,
                                     ui::TableColumn::Alignment alignment) {
  column_formats_[column].setAlignment(MakeQtAlignment(alignment));

  auto cell = table_->cellAt(0, column);
  auto cursor = cell.firstCursorPosition();
  cursor.setBlockFormat(column_formats_[column]);
  cursor.insertText(QString::fromStdWString(title), header_format_);
}

void TableDocumentBuilder::SetCell(int row,
                                   int column,
                                   const base::string16& text) {
  auto cell = table_->cellAt(row + 1, column);
  cell.setFormat(row % 2 == 0 ? row_format_ : alternate_row_format_);
  auto cursor = cell.firstCursorPosition();
  cursor.setBlockFormat(column_formats_[column]);
  cursor.insertText(QString::fromStdWString(text));
}

QTextDocument& TableDocumentBuilder::Build() {
  cursor_.movePosition(QTextCursor::End);
  cursor_.insertBlock();

  return document_;
}

void PrintTable(const PrintTableContext& context) {
  const int row_count = context.model.GetRowCount();
  const int column_count = static_cast<int>(context.columns.size());

  TableDocumentBuilder builder{row_count, column_count};

  for (int i = 0; i < column_count; ++i) {
    builder.SetColumn(i, context.columns[i].title,
                      context.columns[i].alignment);
  }

  for (int row = 0; row < row_count; ++row) {
    for (int column = 0; column < column_count; ++column) {
      auto text = context.model.GetCellText(row, context.columns[column].id);
      builder.SetCell(row, column, text);
    }
  }

  // Print to PDF
  builder.Build().print(&context.print_service.printer);
}

void PrintGrid(const PrintGridContext& context) {
  const int row_count = context.row_model.GetCount();
  const int column_count = context.column_model.GetCount();

  // Add a row header.

  TableDocumentBuilder builder{row_count, 1 + column_count};

  builder.SetColumn(0, {}, ui::TableColumn::Alignment::RIGHT);
  for (int i = 0; i < column_count; ++i) {
    builder.SetColumn(1 + i, context.column_model.GetTitle(i),
                      context.column_model.GetAlignment(i));
  }

  for (int row = 0; row < row_count; ++row) {
    builder.SetCell(row, 0, context.row_model.GetTitle(row));
    for (int column = 0; column < column_count; ++column)
      builder.SetCell(row, 1 + column, context.model.GetCellText(row, column));
  }

  // Print to PDF
  builder.Build().print(&context.print_service.printer);
}
