#pragma once

#include "controls/models/property_model.h"
#include "controls/models/tree_node_model.h"

namespace aui {

class PropertyGroupTreeNode;

class PropertyTreeNode : public aui::TreeNode<PropertyTreeNode> {
 public:
  virtual PropertyGroupTreeNode* AsGroup() { return nullptr; }
};

class PropertyGroupTreeNode : public PropertyTreeNode {
 public:
  PropertyGroupTreeNode(PropertyGroup& property_group,
                        PropertyGroup::ItemType type,
                        int index,
                        std::u16string title);

  void Update();

  // PropertyTreeNode
  virtual PropertyGroupTreeNode* AsGroup() override { return this; }

  // aui::TreeNode
  virtual std::u16string GetText(int column_id) const override;
  virtual aui::Color GetTextColor(int column_id) const override;
  virtual aui::Color GetBackgroundColor(int column_id) const override;
  virtual bool IsSelectable(int column_id) const override { return false; }

  PropertyGroup& property_group;
  PropertyGroup::ItemType type;
  const int index;
  const std::u16string title;
};

class PropertyItemTreeNode : public PropertyTreeNode {
 public:
  PropertyItemTreeNode(PropertyGroup& property_group, int index);

  // aui::TreeNode
  virtual std::u16string GetText(int column_id) const override;
  virtual void SetText(int column_id, const std::u16string& text) override;
  virtual bool IsEditable(int column_id) const override;
  virtual bool IsSelectable(int column_id) const override;
  virtual aui::EditData GetEditData(int column_id) const override;
  virtual void HandleEditButton(int column_id) const override;

  PropertyGroup& property_group;
  const int index;
};

class PropertyTreeModel : public aui::TreeNodeModel<PropertyTreeNode> {
 public:
  using Node = PropertyTreeNode;

  explicit PropertyTreeModel(PropertyModel& property_model);
  ~PropertyTreeModel();

  // aui::TreeModel
  virtual int GetColumnCount() const { return 2; }
  virtual std::u16string GetColumnText(int column_id) const override;

 private:
  PropertyGroupTreeNode* FindGroupNode(PropertyGroup& group);
  PropertyGroupTreeNode* FindGroupNodeHelper(PropertyGroup& group,
                                             PropertyGroupTreeNode& parent);

  void PropertiesChanged(PropertyGroup& group, int first, int count);

  PropertyModel& property_model_;
};

}  // namespace aui
