#include "controls/views/tree.h"

#include "gfx/canvas.h"
#include "skia/ext/skia_utils_win.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/views/widget/widget.h"

#include <atlbase.h>

#include <atlapp.h>
#include <atlctrls.h>
#include <atltypes.h>

Tree::Tree(ui::TreeModel& model) {
  SetController(this);
  SetModel(&model);

  if (model.GetColumnCount() != 1)
    set_custom_painter(this);
}

Tree::~Tree() {
  SetController(nullptr);
  set_custom_painter(nullptr);

  if (images_)
    images_->Destroy();
}

void Tree::LoadIcons(unsigned resource_id, int width, UiColor mask_color) {
  images_.reset(new WTL::CImageList);
  images_->Create(resource_id, width, 0,
                  RGB(SkColorGetR(mask_color), SkColorGetG(mask_color),
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

void Tree::SetCompareHandler(TreeCompareHandler handler) {
  compare_handler_ = std::move(handler);
  SetSorted(!!compare_handler_);
}

int Tree::OnCompare(TreeView& sender, void* left, void* right) {
  return compare_handler_ ? compare_handler_(left, right) : 0;
}

void Tree::ShowContextMenuForView(views::View* source,
                                  const gfx::Point& point) {
  if (context_menu_handler_)
    context_menu_handler_(point);
}

void Tree::SetContextMenuHandler(ContextMenuHandler handler) {
  context_menu_handler_ = std::move(handler);
  set_context_menu_controller(this);
}

std::vector<void*> Tree::GetOrderedNodes(void* root, bool checked) const {
  struct Helper {
    void Traverse(void* root) {
      if (tree.IsChecked(root) != checked)
        return;
      nodes.emplace_back(root);
      for (int i = 0; i < tree.model()->GetChildCount(root); ++i)
        Traverse(tree.model()->GetChild(root, i));
    }

    const Tree& tree;
    const bool checked;
    std::vector<void*> nodes;
  };

  Helper helper{*this, checked};
  helper.Traverse(root);
  return std::move(helper.nodes);
}

void Tree::OnPaintNode(gfx::Canvas* canvas,
                       const gfx::Rect& node_bounds,
                       void* node) {
  const int column_id = 1;

  base::string16 text = model()->GetText(node, column_id);

  SkColor color = model()->GetTextColor(node, column_id);

  const gfx::Font& font = ui::ResourceBundle::GetSharedInstance().GetFont(
      ui::ResourceBundle::BASE_FONT);
  auto size = gfx::Canvas::GetStringSize(text, font);

  gfx::Rect fill_rect = node_bounds;
  fill_rect.set_x(fill_rect.right() - size.width() - 5);
  fill_rect.set_width(size.width());

  SkColor fill_color = model()->GetBackgroundColor(node, column_id);
  if (fill_color != SK_ColorTRANSPARENT)
    canvas->FillRect(fill_rect, fill_color);

  // draw text
  canvas->DrawString(text, font, color, fill_rect,
                     DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
}

void Tree::SetHeaderVisible(bool visible) {}

void Tree::SetRowHeight(int row_height) {}
