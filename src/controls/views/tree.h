#pragma once

#include <memory>

#include "ui/views/controls/tree/tree_view.h"
#include "controls/types.h"

namespace WTL {
template <bool t_bManaged> class CImageListT;
typedef CImageListT<true> CImageListManaged;
}

class Tree : public views::TreeView,
             private views::TreeController {
 public:
  explicit Tree(ui::TreeModel& model);
  virtual ~Tree();

  void LoadIcons(unsigned resource_id, int width, UiColor mask_color);

  int GetSelectionSize() const;
  void SetSelectionChangedHandler(SelectionChangedHandler handler);

  void SetDoubleClickHandler(DoubleClickHandler handler);
  void SetExpandedHandler(TreeExpandedHandler handler);
  void SetCheckedHandler(TreeCheckedHandler handler);
  void SetDragHandler(TreeDragHandler handler);
  void SetEditHandler(TreeEditHandler handler);
  void SetCompareHandler(TreeCompareHandler handler);

 private:
  // TreeController
  virtual void OnDoubleClick(views::TreeView& sender) override;
  virtual void OnShowContextMenu(views::TreeView& sender, const gfx::Point& point) override;
  virtual void OnSelectionChanged(views::TreeView& sender) override;
  virtual void OnDrag(views::TreeView& sender, void* node) override;
  virtual void OnExpanded(views::TreeView& sender, void* node, bool expanded) override;
  virtual void OnChecked(views::TreeView& sender, void* node, bool checked) override;
  virtual bool CanEdit(TreeView& sender, void* node) override;
  virtual int OnCompare(TreeView& sender, void* left, void* right) override;

  DoubleClickHandler double_click_handler_;
  SelectionChangedHandler selection_changed_handler_;
  TreeExpandedHandler expanded_handler_;
  TreeCheckedHandler checked_handler_;
  TreeDragHandler drag_handler_;
  TreeEditHandler edit_handler_;
  TreeCompareHandler compare_handler_;

  std::unique_ptr<WTL::CImageListManaged> images_;
};
