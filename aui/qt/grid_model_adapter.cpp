
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
      column_model_{std::move(column_model)},
      last_row_count_{row_model_->GetCount()},
      last_column_count_{column_model_->GetCount()} {
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
  model_connections_.push_back(model_->SubscribeRowsAdding(
      [this](GridModel& model, int first, int count) {
        OnGridRowsAdding(model, first, count);
      }));
  model_connections_.push_back(model_->SubscribeRowsAdded(
      [this](GridModel& model, int first, int count) {
        OnGridRowsAdded(model, first, count);
      }));
  model_connections_.push_back(model_->SubscribeRowsRemoving(
      [this](GridModel& model, int first, int count) {
        OnGridRowsRemoving(model, first, count);
      }));
  model_connections_.push_back(model_->SubscribeRowsRemoved(
      [this](GridModel& model, int first, int count) {
        OnGridRowsRemoved(model, first, count);
      }));
  model_connections_.push_back(column_model_->SubscribeModelChanged(
      [this](HeaderModel& model) { OnModelChanged(model); }));
  model_connections_.push_back(row_model_->SubscribeModelChanged(
      [this](HeaderModel& model) { OnModelChanged(model); }));
}

int GridModelAdapter::rowCount(const QModelIndex& parent) const {
  // A table has no second level, so a valid parent has no rows. Answering the
  // top-level count for every parent makes the model claim an infinitely deep
  // tree, which is what `QAbstractItemModelTester` reports first.
  return parent.isValid() ? 0 : row_model_->GetCount();
}

int GridModelAdapter::columnCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : column_model_->GetCount();
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
  // Qt asks the *root* index for its flags, and the answer must be the base
  // ones. Passing its row and column (-1, -1) to the model asked "is cell
  // (-1, -1) editable", which `GridModel::IsEditable` answers `true` by
  // default -- so the root came back `ItemIsEditable` and a drop-target check
  // on it read as an editable cell. Found by `QAbstractItemModelTester`.
  if (!index.isValid())
    return flags;
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

void GridModelAdapter::ResetFromModel() {
  // A wholesale change is a reset, never a bare `layoutChanged()`.
  // `QItemSelectionModel` and `QSortFilterProxyModel` both act on the "about
  // to" half: told only afterwards, the selection keeps row numbers that now
  // name other items, and the proxy's mappings are rebuilt while the view
  // still holds `internalPointer()`s into the ones it freed. `GridModel`
  // reports a wholesale change once it has happened, so the pair is emitted
  // back to back -- which is enough, because both halves run before anything
  // else does and the views end up re-reading from scratch.
  beginResetModel();
  last_row_count_ = row_model_->GetCount();
  last_column_count_ = column_model_->GetCount();
  endResetModel();
}

void GridModelAdapter::OnGridModelChanged(GridModel& model) {
  ResetFromModel();
}

void GridModelAdapter::OnGridRangeChanged(GridModel& model,
                                          const GridRange& range) {
  // `GridRange::Rows()` leaves `column_count_` at 0 and `Columns()` leaves
  // `row_count_` at 0, so reading all four fields regardless of `type()` made
  // the bottom-right `index(r, -1)`. `QAbstractItemView::dataChanged` takes
  // its invalid-range branch on that: a full repaint *and*
  // `updateEditorData()` on every open editor, whose
  // `ItemDelegate::setEditorData` re-sets the text from the model and clears
  // `isModified()` -- so a server push for an unrelated row silently discarded
  // what the operator was typing. `SetLooseBounds` exists for exactly this.
  GridRange bounded = range;
  bounded.SetLooseBounds(row_model_->GetCount(), column_model_->GetCount());
  if (bounded.empty())
    return;

  // Naming the roles matters as much as the bounds: an empty role list means
  // "every role", which is another route to repainting and re-reading editors
  // the change did not touch. These are the five `data()` answers.
  dataChanged(index(bounded.row(), bounded.column()),
              index(bounded.last_row(), bounded.last_column()),
              {Qt::DisplayRole, Qt::EditRole, Qt::ForegroundRole,
               Qt::BackgroundRole, Qt::TextAlignmentRole});
}

void GridModelAdapter::OnGridRowsAdding(GridModel& model,
                                        int first,
                                        int count) {
  beginInsertRows(QModelIndex{}, first, first + count - 1);
}

void GridModelAdapter::OnGridRowsAdded(GridModel& model, int first, int count) {
  last_row_count_ = row_model_->GetCount();
  endInsertRows();
}

void GridModelAdapter::OnGridRowsRemoving(GridModel& model,
                                          int first,
                                          int count) {
  beginRemoveRows(QModelIndex{}, first, first + count - 1);
}

void GridModelAdapter::OnGridRowsRemoved(GridModel& model,
                                         int first,
                                         int count) {
  last_row_count_ = row_model_->GetCount();
  endRemoveRows();
}

void GridModelAdapter::OnModelChanged(HeaderModel& model) {
  // `HeaderModel::ModelChanged` is the only signal `SummaryModel::AddColumn` /
  // `DeleteColumn` and `ColumnHeaderModel::SetColumns` emit, and it can change
  // the section *count* -- which `headerDataChanged` cannot express at all,
  // and which it rejects outright when `logicalLast >= count()`. Passing
  // `GetCount()` there made even the title-only case a no-op, and left the
  // header at its old count after a `DeleteColumn`, so the next paint read
  // `GetTitle(old_last)` off a shrunk vector. Announce a count change as a
  // reset -- the change has already happened, so an insert/remove pair cannot
  // be opened honestly -- and only then repaint the titles.
  const bool is_column = &model == column_model_.get();
  const bool is_row = &model == row_model_.get();
  if (!is_column && !is_row)
    return;

  const int count = model.GetCount();
  const int last_count = is_column ? last_column_count_ : last_row_count_;
  if (count != last_count) {
    ResetFromModel();
    return;
  }

  if (count <= 0)
    return;
  headerDataChanged(is_column ? Qt::Horizontal : Qt::Vertical, 0, count - 1);
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
