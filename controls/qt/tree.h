#pragma once

#include <qtreeview.h>

#include "controls/types.h"
#include "qt/tree_model_adapter.h"

namespace ui {
class TreeModel;
}

class Tree : public QTreeView {
 public:
  explicit Tree(ui::TreeModel& model);
  virtual ~Tree();

  TreeModelAdapter& model_adapter() { return *model_adapter_; }

  void SetRootVisible(bool visible);

  void LoadIcons(unsigned resource_id, int width, UiColor mask_color);

  int GetSelectionSize() const;
  void* GetSelectedNode();
  void SelectNode(void* node);
  void SetSelectionChangedHandler(SelectionChangedHandler handler);

  bool IsExpanded(void* node, bool up_to_root) const;
  void SetExpandedHandler(TreeExpandedHandler handler);

  void SetEditable(bool editable);
  void StartEditing(void* node);

  void SetCheckedHandler(TreeCheckedHandler handler);

  void SetDoubleClickHandler(DoubleClickHandler handler);

  void SetCompareHandler(TreeCompareHandler handler);

  void SetContextMenuHandler(ContextMenuHandler handler);

 private:
  std::unique_ptr<TreeModelAdapter> model_adapter_;
};
