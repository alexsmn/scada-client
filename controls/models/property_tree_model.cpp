#include "property_tree_model.h"

#include <cassert>

namespace aui {

// PropertyGroupTreeNode

PropertyGroupTreeNode::PropertyGroupTreeNode(PropertyGroup& property_group,
                                             PropertyGroup::ItemType type,
                                             int index,
                                             std::u16string title)
    : property_group{property_group},
      type{type},
      index{index},
      title{std::move(title)} {
  Update();
}

void PropertyGroupTreeNode::Update() {
  assert(GetChildCount() == 0);

  for (int i = 0; i < property_group.GetCount(); ++i) {
    std::unique_ptr<PropertyTreeNode> node;
    if (auto* subgroup = property_group.GetSubgroup(i))
      node = std::make_unique<PropertyGroupTreeNode>(
          *subgroup, property_group.GetType(i), i, property_group.GetName(i));
    else
      node = std::make_unique<PropertyItemTreeNode>(property_group, i);
    Add(i, std::move(node));
  }
}

std::u16string PropertyGroupTreeNode::GetText(int column_id) const {
  return column_id == 0 ? title : std::u16string{};
}

aui::Color PropertyGroupTreeNode::GetTextColor(int column_id) const {
  if (type == PropertyGroup::ItemType::Category)
    return aui::ColorCode::White;
  return PropertyTreeNode::GetTextColor(column_id);
}

aui::Color PropertyGroupTreeNode::GetBackgroundColor(int column_id) const {
  if (type == PropertyGroup::ItemType::Category)
    return aui::ColorCode::Gray;
  return PropertyTreeNode::GetBackgroundColor(column_id);
}

// PropertyItemTreeNode

PropertyItemTreeNode::PropertyItemTreeNode(PropertyGroup& property_group,
                                           int index)
    : property_group{property_group}, index{index} {}

std::u16string PropertyItemTreeNode::GetText(int column_id) const {
  return column_id == 0 ? property_group.GetName(index)
                        : property_group.GetValue(index);
}

void PropertyItemTreeNode::SetText(int column_id, const std::u16string& text) {
  if (column_id != 1)
    return;

  property_group.SetValue(index, text);
}

bool PropertyItemTreeNode::IsEditable(int column_id) const {
  return column_id == 1;
}

bool PropertyItemTreeNode::IsSelectable(int column_id) const {
  return column_id == 1;
}

aui::EditData PropertyItemTreeNode::GetEditData(int column_id) const {
  return property_group.GetEditData(index);
}

void PropertyItemTreeNode::HandleEditButton(int column_id) const {
  return property_group.HandleEditButton(index);
}

// PropertyTreeModel

PropertyTreeModel::PropertyTreeModel(PropertyModel& property_model)
    : property_model_{property_model} {
  assert(!property_model_.model_changed_handler);
  property_model_.model_changed_handler = [this] {
    if (root()->GetChildCount() != 0)
      Remove(*root(), 0, root()->GetChildCount());
    int count = root()->AsGroup()->property_group.GetCount();
    if (count != 0) {
      TreeNodesAdding(root(), 0, count);
      root()->AsGroup()->Update();
      TreeNodesAdded(root(), 0, count);
    }
  };

  assert(!property_model_.properties_changed_handler);
  property_model_.properties_changed_handler = [this](PropertyGroup& group,
                                                      int first, int count) {
    PropertiesChanged(group, first, count);
  };

  set_root(std::make_unique<PropertyGroupTreeNode>(
      property_model_.GetRootGroup(), PropertyGroup::ItemType::Category, 0,
      std::u16string{}));
}

PropertyTreeModel::~PropertyTreeModel() {
  property_model_.model_changed_handler = nullptr;
  property_model_.properties_changed_handler = nullptr;
}

std::u16string PropertyTreeModel::GetColumnText(int column_id) const {
  return column_id == 0 ? u"Параметр" : u"Значение";
}

PropertyGroupTreeNode* PropertyTreeModel::FindGroupNode(PropertyGroup& group) {
  auto* node = root()->AsGroup();
  return node ? FindGroupNodeHelper(group, *node) : nullptr;
}

PropertyGroupTreeNode* PropertyTreeModel::FindGroupNodeHelper(
    PropertyGroup& group,
    PropertyGroupTreeNode& parent) {
  auto* node = root()->AsGroup();
  for (int i = 0; i < node->GetChildCount(); ++i) {
    if (&node->property_group == &group)
      return node;
  }
  return nullptr;
}

void PropertyTreeModel::PropertiesChanged(PropertyGroup& group,
                                          int first,
                                          int count) {
  auto* node = FindGroupNode(group);
  if (!node)
    return;

  for (int i = 0; i < count; ++i)
    TreeNodeChanged(&node->GetChild(first + i));
}

}  // namespace aui
