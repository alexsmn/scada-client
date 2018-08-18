#pragma once

#include "controls/property_model.h"
#include "ui/base/models/tree_node_model.h"

class PropertyGroupTreeNode;

class PropertyTreeNode : public ui::TreeNode<PropertyTreeNode> {
 public:
  virtual PropertyGroupTreeNode* AsGroup() { return nullptr; }
};

class PropertyGroupTreeNode : public PropertyTreeNode {
 public:
  PropertyGroupTreeNode(PropertyGroup& property_group, base::string16 title);

  // PropertyTreeNode
  virtual PropertyGroupTreeNode* AsGroup() { return this; }

  // ui::TreeNode
  virtual base::string16 GetText(int column_id) const override;

  PropertyGroup& property_group;
  const base::string16 title;
};

class PropertyItemTreeNode : public PropertyTreeNode {
 public:
  PropertyItemTreeNode(PropertyGroup& property_group, int index);

  // ui::TreeNode
  virtual base::string16 GetText(int column_id) const override;
  virtual void SetText(int column_id, const base::string16& text) override;
  virtual bool IsEditable(int column_id) const override;
  virtual ui::EditData GetEditData(int column_id) override;

  PropertyGroup& property_group;
  const int index;
};

class PropertyTreeModel : public ui::TreeNodeModel<PropertyTreeNode> {
 public:
  explicit PropertyTreeModel(PropertyModel& property_model);
  ~PropertyTreeModel();

  // ui::TreeModel
  virtual int GetColumnCount() const { return 2; }
  virtual base::string16 GetColumnText(int column_id) const override;

 private:
  PropertyGroupTreeNode* FindGroupNode(PropertyGroup& group);
  PropertyGroupTreeNode* FindGroupNodeHelper(PropertyGroup& group,
                                             PropertyGroupTreeNode& parent);

  void PropertiesChanged(PropertyGroup& group, int first, int count);

  PropertyModel& property_model_;
};

