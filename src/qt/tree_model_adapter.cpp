#include "qt/tree_model_adapter.h"

#include <cassert>
#include <qbitmap.h>
#include <qpixmap.h>
#include <qsize.h>
#include <windows.h>

#include "base/win/scoped_gdi_object.h"
#include "ui/base/models/tree_model.h"
#include "base/color.h"

#ifdef OS_WIN
#include <qwinfunctions.h>
#endif

TreeModelAdapter::TreeModelAdapter(ui::TreeModel& model)
    : model_(model),
      checkable_(false) {
  model_.AddObserver(*this);
}

TreeModelAdapter::~TreeModelAdapter() {
  model_.RemoveObserver(*this);
}

void TreeModelAdapter::LoadIcons(unsigned resource_id, int width, QColor mask_color) {
  base::win::ScopedBitmap bitmap(::LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(resource_id)));

  QPixmap tile = QtWin::fromHBITMAP(bitmap.get());
  tile.setMask(tile.createMaskFromColor(mask_color));

  for (int x = 0; x < tile.width(); x += width)
    icons_.emplace_back(QIcon(tile.copy(x, 0, width, tile.height())));
}

void* TreeModelAdapter::GetNode(const QModelIndex& index) const {
  return index.internalPointer();
}

QModelIndex TreeModelAdapter::GetNodeIndex(void* node, int column) const {
  int row = GetIndexOf(node);
  return createIndex(row, column, node);
}

QVariant TreeModelAdapter::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation != Qt::Horizontal)
    return QAbstractItemModel::headerData(section, orientation, role);

  switch (role) {
    case Qt::DisplayRole:
      return QString::fromStdWString(model_.GetColumnText(section));
    default:
      return QAbstractItemModel::headerData(section, orientation, role);
  }
}

QModelIndex TreeModelAdapter::index(int row, int column,
                                    const QModelIndex &parent) const {
  if (!hasIndex(row, column, parent))
    return QModelIndex();

  void* parent_node = parent.isValid() ? GetNode(parent) : model_.GetRoot();

  void* child_node = model_.GetChild(parent_node, row);
  return createIndex(row, column, child_node);
}

QModelIndex TreeModelAdapter::parent(const QModelIndex &child) const {
  if (!child.isValid())
    return QModelIndex();

  void* child_node = GetNode(child);
  void* parent_node = model_.GetParent(child_node);

  return parent_node == model_.GetRoot() ?
      QModelIndex() :
      createIndex(GetIndexOf(parent_node), 0, parent_node);
}

int TreeModelAdapter::rowCount(const QModelIndex &parent) const {
  if (parent.column() > 0)
    return 0;

  void* parent_node = parent.isValid() ? GetNode(parent) : model_.GetRoot();
  return model_.GetChildCount(parent_node);
}

int TreeModelAdapter::columnCount(const QModelIndex &parent) const {
  return model_.GetColumnCount();
}

QVariant TreeModelAdapter::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  void* node = GetNode(index);
  assert(node);

  switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      return QString::fromStdWString(model_.GetText(node, index.column()));
    case Qt::TextColorRole:
      return ColorToQt(model_.GetTextColor(node, index.column()));
    case Qt::BackgroundColorRole:
      return ColorToQt(model_.GetBackgroundColor(node, index.column()));
    case Qt::DecorationRole: {
      auto icon_index = index.column() == 0 ? model_.GetIcon(node) : -1;
      return (icon_index >= 0 && icon_index < icons_.size()) ? icons_[icon_index] : QVariant();
    }
    case Qt::CheckStateRole:
      if (!checkable_ || index.column() != 0)
        return QVariant();
      return checked_nodes_.find(node) == checked_nodes_.end() ? Qt::Unchecked : Qt::Checked;
    default:
      return QVariant();
  }
}

bool TreeModelAdapter::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (role != Qt::EditRole)
    return QAbstractItemModel::setData(index, value, role);

  void* node = GetNode(index);
  assert(node);
  model_.SetText(node, index.column(), value.toString().toStdWString());
  return true;
}

Qt::ItemFlags TreeModelAdapter::flags(const QModelIndex &index) const {
  auto flags = QAbstractItemModel::flags(index);
  if (checkable_ && index.column() == 0)
    flags |= Qt::ItemIsUserCheckable;
  if (IsEditable(index.column()))
    flags |= Qt::ItemIsEditable;
  return flags;
}

int TreeModelAdapter::GetIndexOf(void* node) const {
  if (node == model_.GetRoot())
    return 0;

  void* parent_node = model_.GetParent(node);
  for (int i = 0; i < model_.GetChildCount(parent_node); ++i) {
    if (model_.GetChild(parent_node, i) == node)
      return i;
  }

  return -1;
}

void TreeModelAdapter::OnTreeNodesAdded(void* parent, int start, int count) {
  layoutChanged();
}

void TreeModelAdapter::OnTreeNodesDeleted(void* parent, int start, int count) {
  layoutChanged();
}

void TreeModelAdapter::OnTreeNodeChanged(void* node) {
  dataChanged(
      GetNodeIndex(node, 0),
      GetNodeIndex(node, model_.GetColumnCount() - 1));
}

bool TreeModelAdapter::IsEditable(int column_id) const {
  return std::find(editable_column_ids_.begin(), editable_column_ids_.end(), column_id) != editable_column_ids_.end();
}
