#pragma once

#include "base/values.h"
#include "controls/types.h"
#include "controls/wt/item_delegate.h"
#include "controls/wt/tree_model_adapter.h"

#pragma warning(push)
#pragma warning(disable : 4251)
#include <Wt/WSortFilterProxyModel.h>
#include <Wt/WTreeView.h>
#pragma warning(pop)

namespace ui {
class TreeModel;
}

class Tree;

class TreeProxyModel : public Wt::WSortFilterProxyModel {
 public:
  explicit TreeProxyModel(Tree& tree) : tree_{tree} {}

  void SetCompareHandler(TreeCompareHandler handler);

 protected:
  // QSortFilterProxyModel
  virtual bool lessThan(const Wt::WModelIndex& source_left,
                        const Wt::WModelIndex& source_right) const override;

 private:
  Tree& tree_;
  TreeCompareHandler compare_handler_;
};

class Tree : public Wt::WTreeView {
 public:
  explicit Tree(std::shared_ptr<ui::TreeModel> model);
  virtual ~Tree();

  void SetRootVisible(bool visible);
  void SetHeaderVisible(bool visible);

  void LoadIcons(unsigned resource_id, int width, UiColor mask_color);

  std::vector<void*> GetOrderedNodes(void* root, bool checked) const;

  int GetSelectionSize() const;
  void* GetSelectedNode();
  void SelectNode(void* node);
  void SetSelectionChangedHandler(SelectionChangedHandler handler);

  bool IsExpanded(void* node, bool up_to_root) const;
  void SetExpandedHandler(TreeExpandedHandler handler);

  void StartEditing(void* node);

  void SetShowChecks(bool show);
  void SetCheckedHandler(TreeCheckedHandler handler);
  bool IsChecked(void* node) const;
  void SetChecked(void* node, bool checked);
  void SetCheckedNodes(std::set<void*> nodes);

  void SetRowHeight(int row_height);

  void SetDoubleClickHandler(DoubleClickHandler handler);

  void SetSorted(bool sorted);
  void SetCompareHandler(TreeCompareHandler handler);

  void SetContextMenuHandler(ContextMenuHandler handler);

  void SetFocusHandler(FocusHandler handler);

  base::Value SaveState() const;
  void RestoreState(const base::Value& data);

 protected:
  /*virtual void drawBranches(Wt::WPainter* painter,
                            const Wt::WRect& rect,
                            const Wt::WModelIndex& index) const override;*/

 private:
  void* GetNode(const Wt::WModelIndex& index) const;
  Wt::WModelIndex GetIndex(void* node, int column_id) const;

  std::shared_ptr<TreeModelAdapter> model_adapter_;
  std::shared_ptr<TreeProxyModel> proxy_model_;

  FocusHandler focus_handler_;

  friend class TreeProxyModel;
};
