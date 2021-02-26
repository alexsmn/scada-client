#pragma once

#include <memory>

#include "controls/types.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/tree/tree_view.h"

namespace WTL {
template <bool t_bManaged>
class CImageListT;
typedef CImageListT<false> CImageList;
typedef CImageListT<true> CImageListManaged;
}  // namespace WTL

class Tree : public views::TreeView,
             private views::TreeController,
             private views::ContextMenuController,
             private views::TreeView::CustomPainter {
 public:
  explicit Tree(ui::TreeModel& model);
  virtual ~Tree();

  void SetHeaderVisible(bool visible);
  void SetRowHeight(int row_height);

  std::vector<void*> GetOrderedNodes(void* root, bool checked) const;

  void LoadIcons(unsigned resource_id, int width, UiColor mask_color);

  int GetSelectionSize() const;
  void SetSelectionChangedHandler(SelectionChangedHandler handler);

  void SetDoubleClickHandler(DoubleClickHandler handler);
  void SetExpandedHandler(TreeExpandedHandler handler);
  void SetCheckedHandler(TreeCheckedHandler handler);
  void SetDragHandler(TreeDragHandler handler);
  void SetCompareHandler(TreeCompareHandler handler);
  void SetContextMenuHandler(ContextMenuHandler handler);
  void SetFocusHandler(FocusHandler handler) {}

  base::Value SaveState() const { return {}; }
  void RestoreState(const base::Value& data) {}

 private:
  // TreeController
  virtual void OnDoubleClick(views::TreeView& sender) override;
  virtual void OnShowContextMenu(views::TreeView& sender,
                                 const gfx::Point& point) override;
  virtual void OnSelectionChanged(views::TreeView& sender) override;
  virtual void OnDrag(views::TreeView& sender, void* node) override;
  virtual void OnExpanded(views::TreeView& sender,
                          void* node,
                          bool expanded) override;
  virtual void OnChecked(views::TreeView& sender,
                         void* node,
                         bool checked) override;
  virtual int OnCompare(TreeView& sender, void* left, void* right) override;

  // views::ContextMenuController
  virtual void ShowContextMenuForView(views::View* source,
                                      const gfx::Point& point) override;

  // views::TreeView::CustomPainter
  virtual void OnPaintNode(gfx::Canvas* canvas,
                           const gfx::Rect& node_bounds,
                           void* node) override;

  DoubleClickHandler double_click_handler_;
  SelectionChangedHandler selection_changed_handler_;
  TreeExpandedHandler expanded_handler_;
  TreeCheckedHandler checked_handler_;
  TreeDragHandler drag_handler_;
  TreeCompareHandler compare_handler_;
  ContextMenuHandler context_menu_handler_;

  std::unique_ptr<WTL::CImageList> images_;
};
