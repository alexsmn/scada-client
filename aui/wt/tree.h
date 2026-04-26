#pragma once

#include <boost/json.hpp>
#include "aui/color.h"
#include "aui/handlers.h"
#include "aui/wt/item_delegate.h"

#include <Wt/WTreeView.h>

namespace aui {

class TreeModel;
class TreeModelAdapter;
class TreeProxyModel;

class Tree : public Wt::WTreeView {
 public:
  explicit Tree(std::shared_ptr<aui::TreeModel> model);
  virtual ~Tree();

  void SetRootVisible(bool visible);
  void SetHeaderVisible(bool visible);

  void LoadIcons(unsigned resource_id, int width, Color mask_color);

  std::vector<void*> GetOrderedNodes(void* root, bool checked) const;

  int GetSelectionSize() const;
  void* GetSelectedNode();
  void SelectNode(void* node);
  void SetSelectionChangedHandler(SelectionChangedHandler handler);

  bool IsExpanded(void* node, bool up_to_root) const;
  void ExpandNode(void* node);
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

}  // namespace aui
