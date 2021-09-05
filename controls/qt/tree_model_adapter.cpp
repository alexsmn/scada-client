#include "controls/qt/tree_model_adapter.h"

#include "base/win/scoped_gdi_object.h"
#include "controls/color.h"
#include "ui/base/models/tree_model.h"

#include <QBitmap>
#include <QPixmap>
#include <QSize>
#include <cassert>
#include <windows.h>

#ifdef OS_WIN
#include <QtWin>
#endif

TreeModelAdapter::TreeModelAdapter(std::shared_ptr<ui::TreeModel> model)
    : model_{std::move(model)} {
  model_->AddObserver(*this);
}

TreeModelAdapter::~TreeModelAdapter() {
  model_->RemoveObserver(*this);
}

void TreeModelAdapter::LoadIcons(unsigned resource_id,
                                 int width,
                                 QColor mask_color) {
  base::win::ScopedBitmap bitmap(
      ::LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(resource_id)));

  QPixmap tile = QtWin::fromHBITMAP(bitmap.get());
  tile.setMask(tile.createMaskFromColor(mask_color));

  for (int x = 0; x < tile.width(); x += width)
    icons_.emplace_back(QIcon(tile.copy(x, 0, width, tile.height())));
}

void* TreeModelAdapter::GetNode(const QModelIndex& index) const {
  assert(index.isValid());
  return index.internalPointer();
}

QModelIndex TreeModelAdapter::GetNodeIndex(void* node, int column) const {
  int row = GetIndexOf(node);
  return createIndex(row, column, node);
}

QVariant TreeModelAdapter::headerData(int section,
                                      Qt::Orientation orientation,
                                      int role) const {
  if (orientation != Qt::Horizontal)
    return QAbstractItemModel::headerData(section, orientation, role);

  switch (role) {
    case Qt::DisplayRole:
      return QString::fromStdWString(model_->GetColumnText(section));
    case Qt::SizeHintRole: {
      auto size =
          QAbstractItemModel::headerData(section, orientation, role).toSize();
      size.setHeight(20);
      int peferred_width = model_->GetColumnPreferredSize(section);
      if (peferred_width != 0)
        size.setWidth(peferred_width);
      return size;
    }
    default:
      return QAbstractItemModel::headerData(section, orientation, role);
  }
}

QModelIndex TreeModelAdapter::index(int row,
                                    int column,
                                    const QModelIndex& parent) const {
  if (!parent.isValid()) {
    assert(row == 0);
    return GetNodeIndex(model_->GetRoot(), column);
  }

  void* parent_node = GetNode(parent);
  if (row >= model_->GetChildCount(parent_node))
    return {};

  void* child_node = model_->GetChild(parent_node, row);
  return createIndex(row, column, child_node);
}

QModelIndex TreeModelAdapter::parent(const QModelIndex& child) const {
  assert(child.isValid());

  void* child_node = GetNode(child);

  if (child_node == model_->GetRoot())
    return QModelIndex();

  void* parent_node = model_->GetParent(child_node);
  return createIndex(GetIndexOf(parent_node), 0, parent_node);
}

int TreeModelAdapter::rowCount(const QModelIndex& parent) const {
  if (parent.column() > 0)
    return 0;

  if (!parent.isValid())
    return 1;  // root only

  return model_->GetChildCount(GetNode(parent));
}

int TreeModelAdapter::columnCount(const QModelIndex& parent) const {
  return model_->GetColumnCount();
}

QVariant TreeModelAdapter::data(const QModelIndex& index, int role) const {
  assert(index.isValid());

  void* node = GetNode(index);

  switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      return QString::fromStdWString(model_->GetText(node, index.column()));
    case Qt::ForegroundRole:
      return ToQColor(model_->GetTextColor(node, index.column()));
    case Qt::BackgroundRole:
      return ToQColor(model_->GetBackgroundColor(node, index.column()));
    case Qt::DecorationRole: {
      auto icon_index = index.column() == 0 ? model_->GetIcon(node) : -1;
      return (icon_index >= 0 && icon_index < static_cast<int>(icons_.size()))
                 ? icons_[icon_index]
                 : QVariant();
    }
    case Qt::SizeHintRole:
      return QSize{-1, row_height};
    case Qt::CheckStateRole:
      if (!checkable_ || index.column() != 0 || node == model_->GetRoot())
        return QVariant();
      return checked_nodes_.find(node) == checked_nodes_.end() ? Qt::Unchecked
                                                               : Qt::Checked;
    default:
      return QVariant();
  }
}

bool TreeModelAdapter::setData(const QModelIndex& index,
                               const QVariant& value,
                               int role) {
  assert(index.isValid());

  void* node = GetNode(index);

  if (role == Qt::EditRole) {
    model_->SetText(node, index.column(), value.toString().toStdWString());
    return true;

  } else if (role == Qt::CheckStateRole) {
    bool checked = value == Qt::Checked;
    if (checked_handler_)
      checked_handler_(node, checked);
    return true;

  } else {
    return false;
  }
}

Qt::ItemFlags TreeModelAdapter::flags(const QModelIndex& index) const {
  assert(index.isValid());

  auto flags = QAbstractItemModel::flags(index);

  void* node = GetNode(index);

  bool selectable = model_->IsSelectable(node, index.column());
  flags.setFlag(Qt::ItemIsSelectable, selectable);

  bool checkable =
      checkable_ && index.column() == 0 && node != model_->GetRoot();
  flags.setFlag(Qt::ItemIsUserCheckable, checkable);

  bool editable = model_->IsEditable(node, index.column());
  flags.setFlag(Qt::ItemIsEditable, editable);

  return flags;
}

int TreeModelAdapter::GetIndexOf(void* node) const {
  if (node == model_->GetRoot())
    return 0;

  void* parent_node = model_->GetParent(node);
  for (int i = 0; i < model_->GetChildCount(parent_node); ++i) {
    if (model_->GetChild(parent_node, i) == node)
      return i;
  }

  return -1;
}

void TreeModelAdapter::OnTreeNodesAdding(void* parent, int start, int count) {
  auto parent_index = GetNodeIndex(parent, 0);
  beginInsertRows(parent_index, start, start + count - 1);
}

void TreeModelAdapter::OnTreeNodesAdded(void* parent, int start, int count) {
  endInsertRows();
}

void TreeModelAdapter::OnTreeNodesDeleting(void* parent, int start, int count) {
  auto parent_index = GetNodeIndex(parent, 0);
  beginRemoveRows(parent_index, start, start + count - 1);
}

void TreeModelAdapter::OnTreeNodesDeleted(void* parent, int start, int count) {
  endRemoveRows();
}

void TreeModelAdapter::OnTreeNodeChanged(void* node) {
  dataChanged(GetNodeIndex(node, 0),
              GetNodeIndex(node, model_->GetColumnCount() - 1));
}

bool TreeModelAdapter::IsChecked(void* node) const {
  return checked_nodes_.find(node) != checked_nodes_.end();
}

void TreeModelAdapter::SetChecked(void* node, bool checked) {
  bool changed = false;

  if (checked)
    changed = checked_nodes_.emplace(node).second;
  else
    changed = checked_nodes_.erase(node) != 0;

  if (changed) {
    auto index = GetNodeIndex(node, 0);
    dataChanged(index, index, {Qt::CheckStateRole});
  }
}

void TreeModelAdapter::SetCheckedNodes(std::set<void*> nodes) {
  std::vector<void*> changed_nodes;
  if (nodes.empty())
    changed_nodes.reserve(checked_nodes_.size());
  else if (checked_nodes_.empty())
    changed_nodes.reserve(nodes.size());
  std::set_symmetric_difference(nodes.begin(), nodes.end(),
                                checked_nodes_.begin(), checked_nodes_.end(),
                                std::back_inserter(changed_nodes));

  checked_nodes_ = std::move(nodes);

  const QVector<int> roles = {Qt::CheckStateRole};
  for (auto* node : changed_nodes) {
    auto index = GetNodeIndex(node, 0);
    dataChanged(index, index, roles);
  }
}

void TreeModelAdapter::OnTreeModelResetting() {
  beginResetModel();
}

void TreeModelAdapter::OnTreeModelReset() {
  endResetModel();
}

bool TreeModelAdapter::hasChildren(const QModelIndex& parent) const {
  void* node = parent.isValid() ? GetNode(parent) : model_->GetRoot();
  return model_->HasChildren(node);
}

bool TreeModelAdapter::canFetchMore(const QModelIndex& parent) const {
  void* node = parent.isValid() ? GetNode(parent) : model_->GetRoot();
  return model_->CanFetchMore(node);
}

void TreeModelAdapter::fetchMore(const QModelIndex& parent) {
  void* node = parent.isValid() ? GetNode(parent) : model_->GetRoot();
  model_->FetchMore(node);
}
