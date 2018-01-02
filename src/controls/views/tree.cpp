#include "controls/views/tree.h"

#include <atlbase.h>
#include <atltypes.h>
#include <atlapp.h>
#include <atlctrls.h>

#include "skia/ext/skia_utils_win.h"
#include "ui/views/widget/widget.h"

Tree::Tree(ui::TreeModel& model) {
  SetController(this);
  SetModel(&model);
}

Tree::~Tree() {
  SetController(nullptr);
}

void Tree::LoadIcons(unsigned resource_id, int width, UiColor mask_color) {
  images_.reset(new WTL::CImageListManaged);
  images_->Create(resource_id, width, 0, RGB(SkColorGetR(mask_color),
                                             SkColorGetG(mask_color),
                                             SkColorGetB(mask_color)));
  SetIcons(*images_);
}

void Tree::SetDoubleClickHandler(DoubleClickHandler handler) {
  double_click_handler_ = std::move(handler);
}

void Tree::SetSelectionChangedHandler(SelectionChangedHandler handler) {
  selection_changed_handler_ = std::move(handler);
}

void Tree::SetExpandedHandler(TreeExpandedHandler handler) {
  expanded_handler_ = std::move(handler);
}

void Tree::SetCheckedHandler(TreeCheckedHandler handler) {
  checked_handler_ = std::move(handler);
}

void Tree::SetDragHandler(TreeDragHandler handler) {
  drag_handler_ = std::move(handler);
}

void Tree::OnDoubleClick(views::TreeView& sender) {
  if (double_click_handler_)
    double_click_handler_();
}

void Tree::OnShowContextMenu(views::TreeView& sender, const gfx::Point& point) {
}

void Tree::OnSelectionChanged(views::TreeView& sender) {
  if (selection_changed_handler_)
    selection_changed_handler_();
}

void Tree::OnDrag(views::TreeView& sender, void* node) {
  if (drag_handler_)
    drag_handler_(node);
}

void Tree::OnExpanded(views::TreeView& sender, void* node, bool expanded) {
  if (expanded_handler_)
    expanded_handler_(node, expanded);
}

void Tree::OnChecked(views::TreeView& sender, void* node, bool checked) {
  if (checked_handler_)
    checked_handler_(node, checked);
}

int Tree::GetSelectionSize() const {
  return GetSelectedNode() ? 1 : 0;
}

bool Tree::CanEdit(TreeView& sender, void* node) {
  return edit_handler_ && edit_handler_(node);
}

void Tree::SetEditHandler(TreeEditHandler handler) {
  edit_handler_ = std::move(handler);
}

void Tree::SetCompareHandler(TreeCompareHandler handler) {
  compare_handler_ = std::move(handler);
  SetSorted(!!compare_handler_);
}

int Tree::OnCompare(TreeView& sender, void* left, void* right) {
  return compare_handler_ ? compare_handler_(left, right) : 0;
}
