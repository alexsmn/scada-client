#pragma once

#include <QSortFilterProxyModel>
#include <QTreeView>

#include "controls/types.h"
#include "item_delegate.h"
#include "qt/tree_model_adapter.h"

namespace ui {
class TreeModel;
}

class Tree;

class TreeProxyModel : public QSortFilterProxyModel {
 public:
  explicit TreeProxyModel(Tree& tree) : tree_{tree} {}

  TreeCompareHandler compare_handler;

 protected:
  // QSortFilterProxyModel
  virtual bool lessThan(const QModelIndex& source_left,
                        const QModelIndex& source_right) const override;

 private:
  Tree& tree_;
};

class Tree : public QTreeView {
 public:
  explicit Tree(ui::TreeModel& model);
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

 protected:
  virtual void drawBranches(QPainter* painter,
                            const QRect& rect,
                            const QModelIndex& index) const override;

 private:
  void* GetNode(const QModelIndex& index) const;
  QModelIndex GetIndex(void* node, int column_id) const;

  TreeModelAdapter model_adapter_;
  TreeProxyModel proxy_model_;

  ItemDelegate item_delegate_;

  friend class TreeProxyModel;
};
