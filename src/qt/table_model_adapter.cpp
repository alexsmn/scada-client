#include "client/qt/table_model_adapter.h"

#include <QtCore/qsize.h>

#include "ui/base/models/table_model.h"
#include "client/base/color.h"

namespace {

Qt::AlignmentFlag UiAligmentToQt(ui::TableColumn::Alignment alignment) {
  switch (alignment) {
    case ui::TableColumn::LEFT:
      return Qt::AlignLeft;
    case ui::TableColumn::CENTER:
      return Qt::AlignHCenter;
    case ui::TableColumn::RIGHT:
      return Qt::AlignRight;
    default:
      return Qt::AlignLeft;
  }
}

} // namespace

TableModelAdapter::TableModelAdapter(ui::TableModel& model, std::vector<ui::TableColumn> columns)
    : model_(model),
      columns_(std::move(columns)) {
  model_.observers().AddObserver(this);
}

TableModelAdapter::~TableModelAdapter() {
  model_.observers().RemoveObserver(this);
}

int TableModelAdapter::rowCount(const QModelIndex &parent) const {
  return model_.GetRowCount();
}

int TableModelAdapter::columnCount(const QModelIndex &parent) const {
  return static_cast<int>(columns_.size());
}

QVariant TableModelAdapter::data(const QModelIndex &index, int role) const {
  auto& column = columns_[index.column()];

  switch (role) {
    case Qt::TextAlignmentRole:
      return UiAligmentToQt(column.alignment);
  }

  ui::TableCell cell;
  cell.row = index.row();
  cell.column_id = column.id;
  model_.GetCell(cell);

  switch (role) {
    case Qt::DisplayRole:
      return QString::fromStdWString(cell.text);
    case Qt::TextColorRole:
      return ColorToQt(cell.text_color);
    case Qt::BackgroundColorRole:
      return ColorToQt(cell.cell_color);
    default:
      return QVariant();
  }
}

QVariant TableModelAdapter::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation != Qt::Horizontal)
    return QVariant();
  
  auto& column = columns_[section];

  switch (role) {
    case Qt::DisplayRole:
      return QString::fromStdWString(column.title);
    case Qt::SizeHintRole:
      return QSize(column.width, 19);
    default:
      return QVariant();
  }
}

void TableModelAdapter::OnModelChanged() {
  resetInternalData();
  layoutChanged();
}

void TableModelAdapter::OnItemsChanged(int first, int count) {
  dataChanged(
      index(first, 0),
      index(first + count - 1, static_cast<int>(columns_.size()) - 1));
}

void TableModelAdapter::OnItemsAdded(int first, int count) {
  layoutChanged();
}

void TableModelAdapter::OnItemsRemoved(int first, int count) {
  layoutChanged();
}
