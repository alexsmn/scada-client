#pragma once

#include <boost/json.hpp>
#include "aui/color.h"
#include "aui/handlers.h"

#include <QTreeView>
#include <set>

namespace aui {

class TreeModel;

class ItemDelegate;
class TreeModelAdapter;
class TreeProxyModel;

class Tree : public QTreeView {
 public:
  explicit Tree(std::shared_ptr<TreeModel> model);
  ~Tree();

  void SetRootVisible(bool visible);
  void SetHeaderVisible(bool visible);

  void LoadIcons(unsigned resource_id, int width, Color mask_color);

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

  void SetDragHandler(std::vector<std::string> mime_types, DragHandler handler);
  void SetDropHandler(DropHandler handler);

  boost::json::value SaveState() const;
  void RestoreState(const boost::json::value& data);

 protected:
  // QTreeView
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

}  // namespace aui
