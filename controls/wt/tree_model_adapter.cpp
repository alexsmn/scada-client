#include "controls/wt/tree_model_adapter.h"

#include "base/win/scoped_gdi_object.h"
#include "controls/color.h"
#include "ui/base/models/tree_model.h"

#include <cassert>
#include <windows.h>

namespace {

template <class T>
void set_flag(Wt::WFlags<T>& flags, T flag, bool value) {
  if (value)
    flags |= flag;
  else
    flags.clear(flag);
}

}  // namespace

TreeModelAdapter::TreeModelAdapter(std::shared_ptr<ui::TreeModel> model)
    : model_{std::move(model)} {
  model_->AddObserver(*this);
}

TreeModelAdapter::~TreeModelAdapter() {
  model_->RemoveObserver(*this);
}

void TreeModelAdapter::LoadIcons(unsigned resource_id,
                                 int width,
                                 Wt::WColor mask_color) {
  /* base::win::ScopedBitmap bitmap(
       ::LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(resource_id)));

   QPixmap tile = QtWin::fromHBITMAP(bitmap.get());
   tile.setMask(tile.createMaskFromColor(mask_color));

   for (int x = 0; x < tile.width(); x += width)
     icons_.emplace_back(QIcon(tile.copy(x, 0, width, tile.height())));*/
}

void* TreeModelAdapter::GetNode(const Wt::WModelIndex& index) const {
  assert(index.isValid());
  return index.internalPointer();
}

Wt::WModelIndex TreeModelAdapter::GetNodeIndex(void* node, int column) const {
  int row = GetIndexOf(node);
  return createIndex(row, column, node);
}

Wt::cpp17::any TreeModelAdapter::headerData(int section,
                                            Wt::Orientation orientation,
                                            Wt::ItemDataRole role) const {
  if (orientation != Wt::Orientation::Horizontal)
    return Wt::WAbstractItemModel::headerData(section, orientation, role);

  switch (role.value()) {
    case Wt::ItemDataRole::Display:
      return Wt::WString{model_->GetColumnText(section)};
    /*case Wt::ItemDataRole::SizeHint: {
      auto size =
          QAbstractItemModel::headerData(section, orientation, role).toSize();
      size.setHeight(20);
      int peferred_width = model_->GetColumnPreferredSize(section);
      if (peferred_width != 0)
        size.setWidth(peferred_width);
      return size;
    }*/
    default:
      return WAbstractItemModel::headerData(section, orientation, role);
  }
}

Wt::WModelIndex TreeModelAdapter::index(int row,
                                        int column,
                                        const Wt::WModelIndex& parent) const {
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

Wt::WModelIndex TreeModelAdapter::parent(const Wt::WModelIndex& child) const {
  assert(child.isValid());

  void* child_node = GetNode(child);

  if (child_node == model_->GetRoot())
    return Wt::WModelIndex();

  void* parent_node = model_->GetParent(child_node);
  return createIndex(GetIndexOf(parent_node), 0, parent_node);
}

int TreeModelAdapter::rowCount(const Wt::WModelIndex& parent) const {
  if (parent.column() > 0)
    return 0;

  if (!parent.isValid())
    return 1;  // root only

  return model_->GetChildCount(GetNode(parent));
}

int TreeModelAdapter::columnCount(const Wt::WModelIndex& parent) const {
  return model_->GetColumnCount();
}

Wt::cpp17::any TreeModelAdapter::data(const Wt::WModelIndex& index,
                                      Wt::ItemDataRole role) const {
  assert(index.isValid());

  void* node = GetNode(index);

  switch (role.value()) {
    case Wt::ItemDataRole::Display:
    case Wt::ItemDataRole::Edit:
      return Wt::WString{model_->GetText(node, index.column())};
    /*case Wt::ItemDataRole::TextColorRole:
      return ToWt::WColor(model_->GetTextColor(node, index.column()));
    case Wt::ItemDataRole::BackgroundColorRole:
      return ToWt::WColor(model_->GetBackgroundColor(node, index.column()));*/
    /*case Wt::ItemDataRole::Decoration: {
      auto icon_index = index.column() == 0 ? model_->GetIcon(node) : -1;
      return (icon_index >= 0 && icon_index < icons_.size())
                 ? icons_[icon_index]
                 : Wt::cpp17::any();
    }*/
    /*case Wt::ItemDataRole::SizeHintRole:
      return QSize{-1, row_height};*/
    case Wt::ItemDataRole::Checked:
      if (!checkable_ || index.column() != 0 || node == model_->GetRoot())
        return Wt::cpp17::any();
      return checked_nodes_.find(node) != checked_nodes_.end();
    default:
      return Wt::cpp17::any();
  }
}

bool TreeModelAdapter::setData(const Wt::WModelIndex& index,
                               const Wt::cpp17::any& value,
                               Wt::ItemDataRole role) {
  assert(index.isValid());

  void* node = GetNode(index);

  if (role == Wt::ItemDataRole::Edit) {
    model_->SetText(node, index.column(),
                    Wt::cpp17::any_cast<Wt::WString>(value));
    return true;

  } else if (role == Wt::ItemDataRole::Checked) {
    bool checked = Wt::cpp17::any_cast<bool>(value);
    if (checked_handler_)
      checked_handler_(node, checked);
    return true;

  } else {
    return false;
  }
}

Wt::WFlags<Wt::ItemFlag> TreeModelAdapter::flags(
    const Wt::WModelIndex& index) const {
  assert(index.isValid());

  auto flags = WAbstractItemModel::flags(index);

  void* node = GetNode(index);

  bool selectable = model_->IsSelectable(node, index.column());
  set_flag(flags, Wt::ItemFlag::Selectable, selectable);

  bool checkable =
      checkable_ && index.column() == 0 && node != model_->GetRoot();
  set_flag(flags, Wt::ItemFlag::UserCheckable, checkable);

  bool editable = model_->IsEditable(node, index.column());
  set_flag(flags, Wt::ItemFlag::Editable, editable);

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
  dataChanged()(GetNodeIndex(node, 0),
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
    dataChanged()(index, index);
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

  for (auto* node : changed_nodes) {
    auto index = GetNodeIndex(node, 0);
    dataChanged()(index, index);
  }
}

void TreeModelAdapter::OnTreeModelResetting() {
  // beginResetModel();
}

void TreeModelAdapter::OnTreeModelReset() {
  // endResetModel();
}
