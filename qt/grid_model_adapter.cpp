#include "qt/grid_model_adapter.h"

#include <QtCore/qsize.h>

#include "ui/base/models/grid_range.h"
#include "base/qt/color_qt.h"

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

GridModelAdapter::GridModelAdapter(ui::GridModel& model, ui::HeaderModel& row_model, ui::HeaderModel& column_model)
    : model_(model),
      row_model_(row_model),
      column_model_(column_model) {
  model_.observers().AddObserver(this);
}

GridModelAdapter::~GridModelAdapter() {
  model_.observers().RemoveObserver(this);
}

int GridModelAdapter::rowCount(const QModelIndex &parent) const {
  return row_model_.GetCount();
}

int GridModelAdapter::columnCount(const QModelIndex &parent) const {
  return column_model_.GetCount();
}

QVariant GridModelAdapter::data(const QModelIndex &index, int role) const {
  switch (role) {
    case Qt::TextAlignmentRole:
      return column_model_.GetAlignment(index.column());
  }

  ui::GridCell cell;
  cell.row = index.row();
  cell.column = index.column();
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

QVariant GridModelAdapter::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation != Qt::Horizontal)
    return QVariant();
  
  switch (role) {
    case Qt::DisplayRole:
      return QString::fromStdWString(column_model_.GetTitle(section));
    case Qt::SizeHintRole:
      return QSize(column_model_.GetSize(section), 19);
    default:
      return QVariant();
  }
}

void GridModelAdapter::OnGridModelChanged(ui::GridModel& model) {
  resetInternalData();
  layoutChanged();
}

void GridModelAdapter::OnGridRangeChanged(ui::GridModel& model, const ui::GridRange& range) {
  dataChanged(
      index(range.row(), range.column()),
      index(range.row() + range.row_count() - 1, range.column() + range.column_count() - 1));
}

void GridModelAdapter::OnGridRowsAdded(ui::GridModel& model, int first, int count) {
  layoutChanged();
}

void GridModelAdapter::OnGridRowsRemoved(ui::GridModel& model, int first, int count) {
  layoutChanged();
}
