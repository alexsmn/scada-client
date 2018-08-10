#include "property_tree_model.h"

#include <cassert>

void* PropertyTreeModel::GetParent(void* node) {
  return node == this ? nullptr : this;
}

int PropertyTreeModel::GetChildCount(void* parent) {
  if (parent != this)
    return 0;
  return property_model_.GetCount();
}

void* PropertyTreeModel::GetChild(void* parent, int index) {
  if (parent != this)
    return nullptr;
  return IndexToNode(index);
}

base::string16 PropertyTreeModel::GetText(void* node, int column_id) {
  int index = NodeToIndex(node);
  if (index == -1)
    return {};
  return column_id == 0 ? property_model_.GetName(index)
                        : property_model_.GetValue(index);
}

void PropertyTreeModel::SetText(void* node,
                                int column_id,
                                const base::string16& text) {
  if (column_id != 1)
    return;

  int index = NodeToIndex(node);
  if (index == -1)
    return;

  property_model_.SetValue(index, text);
}

void* PropertyTreeModel::IndexToNode(int index) const {
  return reinterpret_cast<void*>(index + 1);
}

base::string16 PropertyTreeModel::GetColumnText(int column_id) const {
  return column_id == 0 ? L"Параметр" : L"Значение";
}

bool PropertyTreeModel::IsEditable(void* node, int column_id) const {
  return column_id == 1;
}

ui::EditData PropertyTreeModel::GetEditData(void* node, int column_id) {
  int index = NodeToIndex(node);
  if (index == -1)
    return {};

  return property_model_.GetEditData(index);
}

void PropertyTreeModel::PropertiesChanged(int first, int count) {
  for (int i = 0; i < count; ++i)
    TreeNodeChanged(IndexToNode(first + i));
}

int PropertyTreeModel::NodeToIndex(void* node) const {
  return reinterpret_cast<int>(node) - 1;
}

PropertyTreeModel::PropertyTreeModel(PropertyModel& property_model)
    : property_model_{property_model} {
  assert(!property_model_.properties_changed_handler);
  property_model_.properties_changed_handler = [this](int first, int count) {
    PropertiesChanged(first, count);
  };
}

PropertyTreeModel::~PropertyTreeModel() {
  property_model_.properties_changed_handler = nullptr;
}
