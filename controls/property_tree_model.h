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
  PropertyGroupTreeNode(PropertyGroup& property_group,
                        PropertyGroup::ItemType type,
                        int index,
                        std::u16string title);

  void Update();

  // PropertyTreeNode
  virtual PropertyGroupTreeNode* AsGroup() override { return this; }

  // ui::TreeNode
  virtual std::u16string GetText(int column_id) const override;
  virtual SkColor GetTextColor(int column_id) const override;
  virtual SkColor GetBackgroundColor(int column_id) const override;
  virtual bool IsSelectable(int column_id) const override { return false; }

  PropertyGroup& property_group;
  PropertyGroup::ItemType type;
  const int index;
  const std::u16string title;
};

class PropertyItemTreeNode : public PropertyTreeNode {
 public:
  PropertyItemTreeNode(PropertyGroup& property_group, int index);

  // ui::TreeNode
  virtual std::u16string GetText(int column_id) const override;
  virtual void SetText(int column_id, const std::u16string& text) override;
  virtual bool IsEditable(int column_id) const override;
  virtual bool IsSelectable(int column_id) const override;
  virtual ui::EditData GetEditData(int column_id) override;

  PropertyGroup& property_group;
  const int index;
};

class PropertyTreeModel : public ui::TreeNodeModel<PropertyTreeNode> {
 public:
  using Node = PropertyTreeNode;

  explicit PropertyTreeModel(PropertyModel& property_model);
  ~PropertyTreeModel();

  // ui::TreeModel
  virtual int GetColumnCount() const { return 2; }
  virtual std::u16string GetColumnText(int column_id) const override;

 private:
  PropertyGroupTreeNode* FindGroupNode(PropertyGroup& group);
  PropertyGroupTreeNode* FindGroupNodeHelper(PropertyGroup& group,
                                             PropertyGroupTreeNode& parent);

  void PropertiesChanged(PropertyGroup& group, int first, int count);

  PropertyModel& property_model_;
};
