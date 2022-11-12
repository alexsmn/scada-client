#include "controls/qt/table_model_adapter.h"

#include "controls/color.h"
#include "controls/models/table_model.h"
#include "controls/qt/image_util.h"

#include <QSize>

namespace {

Qt::AlignmentFlag AuiAligmentToQt(aui::TableColumn::Alignment alignment) {
  switch (alignment) {
    case aui::TableColumn::LEFT:
      return Qt::AlignLeft;
    case aui::TableColumn::CENTER:
      return Qt::AlignHCenter;
    case aui::TableColumn::RIGHT:
      return Qt::AlignRight;
    default:
      return Qt::AlignLeft;
  }
}

}  // namespace

TableModelAdapter::TableModelAdapter(std::shared_ptr<aui::TableModel> model,
                                     std::vector<aui::TableColumn> columns)
    : model_{std::move(model)}, columns_(std::move(columns)) {
  model_->observers().AddObserver(this);
}

TableModelAdapter::~TableModelAdapter() {
  model_->observers().RemoveObserver(this);
}

void TableModelAdapter::LoadIcons(unsigned resource_id,
                                  int width,
                                  QColor mask_color) {
  icons_ = ::LoadIcons(resource_id, width, mask_color);
}

int TableModelAdapter::rowCount(const QModelIndex& parent) const {
  return model_->GetRowCount();
}

int TableModelAdapter::columnCount(const QModelIndex& parent) const {
  return static_cast<int>(columns_.size());
}

QVariant TableModelAdapter::data(const QModelIndex& index, int role) const {
  auto& column = columns_[index.column()];

  switch (role) {
    case Qt::TextAlignmentRole:
      return AuiAligmentToQt(column.alignment);
    case Qt::ToolTipRole:
      return QString::fromStdU16String(
          model_->GetTooltip(index.row(), column.id));
  }

  aui::TableCell cell;
  cell.row = index.row();
  cell.column_id = column.id;
  model_->GetCell(cell);

  switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      return QString::fromStdU16String(cell.text);
    case Qt::ForegroundRole:
      return cell.text_color.qcolor();
    case Qt::BackgroundRole:
      return cell.cell_color.qcolor();
    case Qt::DecorationRole:
      return (cell.icon_index >= 0 &&
              cell.icon_index < static_cast<int>(icons_.size()))
                 ? icons_[cell.icon_index]
                 : QVariant();
    default:
      return QVariant();
  }
}

bool TableModelAdapter::setData(const QModelIndex& index,
                                const QVariant& value,
                                int role) {
  switch (role) {
    case Qt::EditRole:
      return model_->SetCellText(index.row(), columns_[index.column()].id,
                                 value.toString().toStdU16String());

    default:
      return false;
  }
}

QVariant TableModelAdapter::headerData(int section,
                                       Qt::Orientation orientation,
                                       int role) const {
  if (orientation != Qt::Horizontal)
    return QVariant();

  auto& column = columns_[section];

  switch (role) {
    case Qt::DisplayRole:
      return QString::fromStdU16String(column.title);
    default:
      return QVariant();
  }
}

Qt::ItemFlags TableModelAdapter::flags(const QModelIndex& index) const {
  auto flags = QAbstractItemModel::flags(index);
  if (model_->IsEditable(index.row(), columns_[index.column()].id))
    flags |= Qt::ItemIsEditable;
  return flags;
}

void TableModelAdapter::sort(int column, Qt::SortOrder order) {
  model_->Sort(columns_[column].id, order == Qt::AscendingOrder);
}

void TableModelAdapter::OnModelChanged() {
  resetInternalData();
  layoutChanged();
}

void TableModelAdapter::OnItemsChanged(int first, int count) {
  dataChanged(index(first, 0),
              index(first + count - 1, static_cast<int>(columns_.size()) - 1));
}

void TableModelAdapter::OnItemsAdding(int first, int count) {
  beginInsertRows({}, first, first + count - 1);
}

void TableModelAdapter::OnItemsAdded(int first, int count) {
  endInsertRows();
}

void TableModelAdapter::OnItemsRemoving(int first, int count) {
  beginRemoveRows({}, first, first + count - 1);
}

void TableModelAdapter::OnItemsRemoved(int first, int count) {
  endRemoveRows();
}

QStringList TableModelAdapter::mimeTypes() const {
  return QAbstractItemModel::mimeTypes();
}

QMimeData* TableModelAdapter::mimeData(const QModelIndexList& indexes) const {
  return QAbstractItemModel::mimeData(indexes);
}
