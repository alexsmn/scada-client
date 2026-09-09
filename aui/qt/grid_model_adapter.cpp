
#include "aui/qt/grid_model_adapter.h"

#include "aui/color.h"
#include "aui/models/grid_range.h"
#include "aui/severity_colors.h"
#include "base/check.h"

#include <QMimeData>
#include <QSize>

namespace scada::aui {

namespace {

// TODO: Combine with Table.
Qt::AlignmentFlag AuiAligmentToQt(TableColumn::Alignment alignment) {
  switch (alignment) {
    case TableColumn::LEFT:
      return Qt::AlignLeft;
    case TableColumn::CENTER:
      return Qt::AlignHCenter;
    case TableColumn::RIGHT:
      return Qt::AlignRight;
    default:
      return Qt::AlignLeft;
  }
}

}  // namespace

GridModelAdapter::GridModelAdapter(std::shared_ptr<GridModel> model,
                                   std::shared_ptr<HeaderModel> row_model,
                                   std::shared_ptr<HeaderModel> column_model)
    : model_{std::move(model)},
      row_model_{std::move(row_model)},
      column_model_{std::move(column_model)} {
  ConnectModels();
}

GridModelAdapter::~GridModelAdapter() = default;

void GridModelAdapter::ConnectModels() {
  model_connections_.push_back(model_->SubscribeModelChanged(
      [this](GridModel& model) { OnGridModelChanged(model); }));
  model_connections_.push_back(model_->SubscribeRangeChanged(
      [this](GridModel& model, const GridRange& range) {
        OnGridRangeChanged(model, range);
      }));
  model_connections_.push_back(model_->SubscribeRowsAdded(
      [this](GridModel& model, int first, int count) {
        OnGridRowsAdded(model, first, count);
      }));
  model_connections_.push_back(model_->SubscribeRowsRemoved(
      [this](GridModel& model, int first, int count) {
        OnGridRowsRemoved(model, first, count);
      }));
  model_connections_.push_back(column_model_->SubscribeModelChanged(
      [this](HeaderModel& model) { OnModelChanged(model); }));
}

int GridModelAdapter::rowCount(const QModelIndex& parent) const {
  return row_model_->GetCount();
}

int GridModelAdapter::columnCount(const QModelIndex& parent) const {
  return column_model_->GetCount();
}

QVariant GridModelAdapter::data(const QModelIndex& index, int role) const {
  // Decide by role before touching the model: a delegate asks for seven roles
  // per paint and per sizeHint, this answers five, and GetCell formats the
  // value each time it is called.
  switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case Qt::ForegroundRole:
    case Qt::BackgroundRole:
    case Qt::TextAlignmentRole:
      break;
    default:
      return QVariant();
  }

  GridCell cell;
  cell.row = index.row();
  cell.column = index.column();
  model_->GetCell(cell);

  // A transparent colour means "unstyled": the cell falls through to the theme
  // palette. A cell with an explicit background but default text derives a
  // contrasting text colour, so a semantically light cell (read-only grey,
  // blink yellow) stays readable on the dark theme.
  switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      return QString::fromStdU16String(cell.text);
    case Qt::ForegroundRole:
      if (!IsTransparent(cell.text_color))
        return cell.text_color.qcolor();
      if (!IsTransparent(cell.cell_color)) {
        return cell.cell_color.qcolor().lightness() >= 128 ? QColor{Qt::black}
                                                           : QColor{Qt::white};
      }
      return QVariant();
    case Qt::BackgroundRole:
      if (!IsTransparent(cell.cell_color))
        return cell.cell_color.qcolor();
      return QVariant();
    case Qt::TextAlignmentRole:
      // The cell's own alignment wins over its column's. Qt needs its own
      // flags here: this used to hand back the aui enum raw, which Qt reads as
      // a flag mask — LEFT(0) meant "no alignment", and RIGHT and CENTER came
      // out as AlignLeft and AlignRight. Every grid in the client was
      // effectively left-aligned, including the summary's right-aligned value
      // columns, and the spreadsheet's per-cell alignment had nowhere to go at
      // all.
      return QVariant::fromValue(
          AuiAligmentToQt(cell.alignment.value_or(
              column_model_->GetAlignment(index.column()))) |
          Qt::AlignVCenter);
    default:
      return QVariant();
  }
}

Qt::ItemFlags GridModelAdapter::flags(const QModelIndex& index) const {
  auto flags = QAbstractTableModel::flags(index);
  if (model_->IsEditable(index.row(), index.column()))
    flags |= Qt::ItemIsEditable;
  return flags;
}

QVariant GridModelAdapter::headerData(int section,
                                      Qt::Orientation orientation,
                                      int role) const {
  if (orientation == Qt::Horizontal) {
    switch (role) {
      case Qt::DisplayRole:
        return QString::fromStdU16String(column_model_->GetTitle(section));
      case Qt::SizeHintRole:
        return QSize(column_model_->GetSize(section), 19);
      default:
        return QVariant();
    }

  } else if (orientation == Qt::Vertical) {
    switch (role) {
      case Qt::DisplayRole:
        return QString::fromStdU16String(row_model_->GetTitle(section));
      default:
        return QVariant();
    }

  } else {
    base::NotReached();
  }
}

bool GridModelAdapter::setData(const QModelIndex& index,
                               const QVariant& value,
                               int role) {
  return model_->SetCellText(index.row(), index.column(),
                             value.toString().toStdU16String());
}

void GridModelAdapter::OnGridModelChanged(GridModel& model) {
  resetInternalData();
  layoutChanged();
}

void GridModelAdapter::OnGridRangeChanged(GridModel& model,
                                          const GridRange& range) {
  dataChanged(index(range.row(), range.column()),
              index(range.row() + range.row_count() - 1,
                    range.column() + range.column_count() - 1));
}

void GridModelAdapter::OnGridRowsAdded(GridModel& model, int first, int count) {
  layoutChanged();
}

void GridModelAdapter::OnGridRowsRemoved(GridModel& model,
                                         int first,
                                         int count) {
  layoutChanged();
}

void GridModelAdapter::OnModelChanged(HeaderModel& model) {
  if (&model == column_model_.get())
    headerDataChanged(Qt::Horizontal, 0, model.GetCount());
  else if (&model == row_model_.get())
    headerDataChanged(Qt::Vertical, 0, model.GetCount());
}

QStringList GridModelAdapter::mimeTypes() const {
  return {"text/plain"};
}

QMimeData* GridModelAdapter::mimeData(const QModelIndexList& indexes) const {
  QMimeData* mime_data = new QMimeData;
  mime_data->setData("text/plain",
                     QString::fromStdU16String(GetCsvData(indexes)).toUtf8());
  return mime_data;
}

std::u16string GridModelAdapter::GetCsvData(
    const QModelIndexList& indexes) const {
  // Stable sort by rows.
  auto sorted_indexes = indexes;
  std::stable_sort(sorted_indexes.begin(), sorted_indexes.end(),
                   [](auto& a, auto& b) { return a.row() < b.row(); });

  std::u16string csv;
  int next_index = 0;
  while (next_index < sorted_indexes.size()) {
    int row_index = sorted_indexes[next_index].row();
    bool first_in_row = true;
    while (next_index < sorted_indexes.size() &&
           sorted_indexes[next_index].row() == row_index) {
      if (!first_in_row)
        csv += u',';
      first_in_row = false;
      csv += model_->GetCellText(sorted_indexes[next_index].row(),
                                 sorted_indexes[next_index].column());
      ++next_index;
    }
    csv += u'\n';
  }
  return csv;
}

}  // namespace scada::aui
