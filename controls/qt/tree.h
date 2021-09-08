#pragma once

#include "base/values.h"
#include "controls/handlers.h"

#include <QTreeView>

namespace ui {
class TreeModel;
}

class ItemDelegate;
class Tree;
class TreeModelAdapter;
class TreeProxyModel;

class Tree : public QTreeView {
 public:
  explicit Tree(std::shared_ptr<ui::TreeModel> model);
  ~Tree();

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
  void SetFocusHandler(FocusHandler handler) {}

  base::Value SaveState() const;
  void RestoreState(const base::Value& data);

 protected:
  virtual void drawBranches(QPainter* painter,
                            const QRect& rect,
                            const QModelIndex& index) const override;

 private:
  void* GetNode(const QModelIndex& index) const;
  QModelIndex GetIndex(void* node, int column_id) const;

  std::unique_ptr<TreeModelAdapter> model_adapter_;
  std::unique_ptr<TreeProxyModel> proxy_model_;

  std::unique_ptr<ItemDelegate> item_delegate_;

  friend class TreeProxyModel;
};
