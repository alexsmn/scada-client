#include "qt/grid_model_adapter.h"

#include <QtCore/qsize.h>

#include "controls/color.h"
#include "ui/base/models/grid_range.h"

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

}  // namespace

GridModelAdapter::GridModelAdapter(ui::GridModel& model,
                                   ui::HeaderModel& row_model,
                                   ui::HeaderModel& column_model)
    : model_(model), row_model_(row_model), column_model_(column_model) {
  model_.observers().AddObserver(this);
  column_model_.observers().AddObserver(this);
}

GridModelAdapter::~GridModelAdapter() {
  model_.observers().RemoveObserver(this);
  column_model_.observers().RemoveObserver(this);
}

int GridModelAdapter::rowCount(const QModelIndex& parent) const {
  return row_model_.GetCount();
}

int GridModelAdapter::columnCount(const QModelIndex& parent) const {
  return column_model_.GetCount();
}

QVariant GridModelAdapter::data(const QModelIndex& index, int role) const {
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
    case Qt::EditRole:
      return QString::fromStdWString(cell.text);
    case Qt::ForegroundRole:
      return ToQColor(cell.text_color);
    case Qt::BackgroundRole:
      return ToQColor(cell.cell_color);
    default:
      return QVariant();
  }
}

Qt::ItemFlags GridModelAdapter::flags(const QModelIndex& index) const {
  auto flags = QAbstractTableModel::flags(index);
  if (model_.IsEditable(index.row(), index.column()))
    flags |= Qt::ItemIsEditable;
  return flags;
}

QVariant GridModelAdapter::headerData(int section,
                                      Qt::Orientation orientation,
                                      int role) const {
  if (orientation == Qt::Horizontal) {
    switch (role) {
      case Qt::DisplayRole:
        return QString::fromStdWString(column_model_.GetTitle(section));
      case Qt::SizeHintRole:
        return QSize(column_model_.GetSize(section), 19);
      default:
        return QVariant();
    }

  } else if (orientation == Qt::Vertical) {
    switch (role) {
      case Qt::DisplayRole:
        return QString::fromStdWString(row_model_.GetTitle(section));
      default:
        return QVariant();
    }

  } else {
    assert(false);
    return QVariant();
  }
}

bool GridModelAdapter::setData(const QModelIndex& index,
                               const QVariant& value,
                               int role) {
  return model_.SetCellText(index.row(), index.column(),
                            value.toString().toStdWString());
}

void GridModelAdapter::OnGridModelChanged(ui::GridModel& model) {
  resetInternalData();
  layoutChanged();
}

void GridModelAdapter::OnGridRangeChanged(ui::GridModel& model,
                                          const ui::GridRange& range) {
  dataChanged(index(range.row(), range.column()),
              index(range.row() + range.row_count() - 1,
                    range.column() + range.column_count() - 1));
}

void GridModelAdapter::OnGridRowsAdded(ui::GridModel& model,
                                       int first,
                                       int count) {
  layoutChanged();
}

void GridModelAdapter::OnGridRowsRemoved(ui::GridModel& model,
                                         int first,
                                         int count) {
  layoutChanged();
}

void GridModelAdapter::OnModelChanged(ui::HeaderModel& model) {
  if (&model == &column_model_)
    headerDataChanged(Qt::Horizontal, 0, model.GetCount());
  else if (&model == &row_model_)
    headerDataChanged(Qt::Vertical, 0, model.GetCount());
}
