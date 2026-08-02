
#include "aui/qt/table_model_adapter.h"

#include "aui/color.h"
#include "aui/models/table_model.h"
#include "aui/qt/image_util.h"
#include "aui/qt/theme_qt.h"
#include "base/check.h"

#include <QSize>

namespace scada::aui {

namespace {

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

bool IsTransparent(Color color) {
  return color.rgba().a == 0;
}

}  // namespace

TableModelAdapter::TableModelAdapter(std::shared_ptr<TableModel> model,
                                     std::vector<TableColumn> columns)
    : model_{std::move(model)}, columns_(std::move(columns)) {
  ConnectModel();
}

TableModelAdapter::~TableModelAdapter() = default;

void TableModelAdapter::ConnectModel() {
  model_connections_.push_back(
      model_->SubscribeModelChanged([this] { OnModelChanged(); }));
  model_connections_.push_back(model_->SubscribeItemsChanged(
      [this](int first, int count) { OnItemsChanged(first, count); }));
  model_connections_.push_back(model_->SubscribeItemsAdding(
      [this](int first, int count) { OnItemsAdding(first, count); }));
  model_connections_.push_back(model_->SubscribeItemsAdded(
      [this](int first, int count) { OnItemsAdded(first, count); }));
  model_connections_.push_back(model_->SubscribeItemsRemoving(
      [this](int first, int count) { OnItemsRemoving(first, count); }));
  model_connections_.push_back(model_->SubscribeItemsRemoved(
      [this](int first, int count) { OnItemsRemoved(first, count); }));
}

void TableModelAdapter::LoadGlyphs(
    std::span<const std::string_view> resource_paths,
    int size,
    Color tint,
    qreal device_pixel_ratio) {
  glyph_paths_.assign(resource_paths.begin(), resource_paths.end());
  glyph_size_ = size;
  icons_ = ::LoadTintedGlyphs(resource_paths, size, tint.qcolor(),
                              device_pixel_ratio);
}

void TableModelAdapter::RetintGlyphs(Color tint, qreal device_pixel_ratio) {
  if (glyph_paths_.empty())
    return;

  std::vector<std::string_view> paths;
  paths.reserve(glyph_paths_.size());
  for (const std::string& path : glyph_paths_)
    paths.emplace_back(path);
  icons_ =
      ::LoadTintedGlyphs(paths, glyph_size_, tint.qcolor(), device_pixel_ratio);
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
      // Horizontal from the column, vertical always centred — Qt's own default
      // for item views, and what the grid adapter renders. Returning the
      // horizontal flag alone leaves the vertical bits zero, which Qt reads as
      // AlignTop, so table rows sat a pixel higher than grid rows throughout
      // the client.
      return QVariant::fromValue(AuiAligmentToQt(column.alignment) |
                                 Qt::AlignVCenter);
    case Qt::ToolTipRole:
      return QString::fromStdU16String(
          model_->GetTooltip(index.row(), column.id));
    case Qt::FontRole:
      // Value and timestamp columns render in the design-system monospace
      // font so digits stay tabular as they update (design-language.md §3).
      // `MonoValueFont` is empty under the legacy theme, keeping the default.
      if (column.monospace ||
          column.data_type == TableColumn::DataType::DateTime) {
        if (std::optional<QFont> font = MonoValueFont())
          return *font;
      }
      return QVariant();
  }

  TableCell cell;
  cell.row = index.row();
  cell.column_id = column.id;
  model_->GetCell(cell);

  switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      return QString::fromStdU16String(cell.text);
    case Qt::ForegroundRole:
      return IsTransparent(cell.text_color) ? QVariant{}
                                            : cell.text_color.qcolor();
    case Qt::BackgroundRole:
      return IsTransparent(cell.cell_color) ? QVariant{}
                                            : cell.cell_color.qcolor();
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
  base::Check(count > 0);
  dataChanged(index(first, 0),
              index(first + count - 1, static_cast<int>(columns_.size()) - 1));
}

void TableModelAdapter::OnItemsAdding(int first, int count) {
  base::Check(count > 0);
  beginInsertRows({}, first, first + count - 1);
}

void TableModelAdapter::OnItemsAdded(int first, int count) {
  base::Check(count > 0);
  endInsertRows();
}

void TableModelAdapter::OnItemsRemoving(int first, int count) {
  base::Check(count > 0);
  beginRemoveRows({}, first, first + count - 1);
}

void TableModelAdapter::OnItemsRemoved(int first, int count) {
  base::Check(count > 0);
  endRemoveRows();
}

QStringList TableModelAdapter::mimeTypes() const {
  return QAbstractItemModel::mimeTypes();
}

QMimeData* TableModelAdapter::mimeData(const QModelIndexList& indexes) const {
  return QAbstractItemModel::mimeData(indexes);
}

}  // namespace scada::aui
