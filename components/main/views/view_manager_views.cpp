#include "components/main/views/view_manager_views.h"

#include "base/auto_reset.h"
#include "components/main/opened_view.h"
#include "components/main/view_manager_delegate.h"
#include "services/page.h"
#include "ui/views/controls/split_view.h"
#include "ui/views/widget/root_view.h"
#include "ui/views/widget/widget.h"
#include "window_info.h"

ViewManagerViews::ViewManagerViews(views::View& parent_view,
                                   ViewManagerDelegate& delegate)
    : ViewManager{delegate} {
  dock_container_ = std::make_unique<views::MultiSplitView>();
  dock_container_->SetController(this);
  parent_view.AddChildView(dock_container_.get());
}

ViewManagerViews::~ViewManagerViews() {
  if (focus_manager_)
    focus_manager_->RemoveFocusChangeListener(*this);
}

void ViewManagerViews::Init() {
  focus_manager_ = &dock_container_->GetWidget()->focus_manager();
  focus_manager_->AddFocusChangeListener(*this);
}

views::View& ViewManagerViews::GetView() {
  return *dock_container_;
}

void ViewManagerViews::SetViewTitle(OpenedView& view,
                                    const std::wstring& title) {
  if (view.view())
    dock_container_->SetViewTitle(*view.view(), title);
}

void ViewManagerViews::OpenLayout(Page& page, const PageLayout& layout) {
  OpenLayoutBlock(layout.main, dock_container_->root_pane());

  // Open windows not opended by layout.
  for (auto* opened_view : views_) {
    if (!IsViewAdded(*opened_view))
      AddView(*opened_view);
  }
}

void ViewManagerViews::OpenLayoutBlock(const PageLayoutBlock& block,
                                       views::MultiSplitPane& pane) {
  if (block.type == PageLayoutBlock::SPLIT) {
    views::ViewSide dock_side =
        block.horz ? views::DOCK_BOTTOM : views::DOCK_RIGHT;
    int pane_percent_size = block.pos > 0 ? 100 - block.pos : 50;
    views::MultiSplitPane& new_pane =
        dock_container_->SplitPane(pane, dock_side, pane_percent_size);

    OpenLayoutBlock(*block.left, pane);
    OpenLayoutBlock(*block.right, new_pane);

  } else if (block.type == PageLayoutBlock::PANE) {
    for (size_t i = 0; i < block.wins.size(); ++i) {
      OpenedView* view = FindViewByID(block.wins[i]);
      if (view && !IsViewAdded(*view)) {
        std::wstring title = view->GetWindowTitle();
        dock_container_->AddView(pane, views::DOCK_CENTER, *view->view(), title,
                                 gfx::Image() /*view->image()*/, 0);
        added_views_.emplace_back(view);
      }
    }

    if (block.central)
      dock_container_->SetCentralPane(pane);
  }
}

void ViewManagerViews::SaveLayout(PageLayout& layout) {
  SaveLayoutBlock(layout.main, dock_container_->root_pane());
}

void ViewManagerViews::SaveLayoutBlock(PageLayoutBlock& block,
                                       views::MultiSplitPane& pane) {
  assert(block.empty());

  if (pane.is_split()) {
    block.split(pane.is_split_vertical());
    block.pos = pane.split_position();
    if (pane.split_pane(0))
      SaveLayoutBlock(block.top(), *pane.split_pane(0));
    if (pane.split_pane(1))
      SaveLayoutBlock(block.bottom(), *pane.split_pane(1));

  } else {
    views::TabView& tab_view = pane.tab_view();
    for (int i = 0; i < tab_view.tab_count(); ++i) {
      if (OpenedView* view = FindViewByViewsView(&tab_view.GetTabView(i)))
        block.add(view->window_id());
    }
  }

  if (&pane == &dock_container_->central_pane())
    block.central = true;
}

void ViewManagerViews::ActivateView(OpenedView& view) {
  dock_container_->MakeViewVisible(*view.view());
  view.view()->RequestFocus();
}

OpenedView* ViewManagerViews::FindViewByViewsView(views::View* view) {
  for (; view; view = view->parent()) {
    // Check this view is top level view.
    auto i = std::find_if(views_.begin(), views_.end(),
                          [view](OpenedView* opened_view) {
                            return opened_view->view() == view;
                          });
    if (i != views_.end())
      return *i;
  }
  return nullptr;
}

OpenedView* ViewManagerViews::FindFirstDataView(
    views::MultiSplitPane& from_pane) {
  if (from_pane.is_split()) {
    for (int i = 0; i < 2; ++i) {
      if (views::MultiSplitPane* sub_pane = from_pane.split_pane(i)) {
        if (auto* view = FindFirstDataView(*sub_pane))
          return view;
      }
    }
    return nullptr;

  } else {
    views::TabView& tab_view = from_pane.tab_view();
    for (int i = 0; i < tab_view.tab_count(); ++i) {
      OpenedView* v = FindViewByViewsView(&tab_view.GetTabView(i));
      if (v && !v->window_info().is_pane())
        return v;
    }
    return nullptr;
  }
}

void ViewManagerViews::OnShowViewTabContextMenu(views::View& view,
                                                gfx::Point point) {
  OpenedView* v = FindViewByViewsView(&view);
  if (!v)
    return;

  ActivateView(*v);

  delegate_.OnShowTabPopupMenu(*v, point);
}

void ViewManagerViews::AddView(OpenedView& view) {
  assert(view.view());
  assert(!IsViewAdded(view));

  views::MultiSplitPane* pane = nullptr;
  views::ViewSide side = views::DOCK_CENTER;
  int percent_size = 50;

  if (!pane && view.window_info().is_pane()) {
    percent_size = view.window_info().dock_bottom()
                       ? view.window_info().cy * 100 / 768
                       : view.window_info().cx * 100 / 1024;
    side = view.window_info().dock_bottom() ? views::DOCK_BOTTOM
                                            : views::DOCK_LEFT;
    pane = dock_container_->FindDockPane(side);
    /*if (pane)
      side = views::DOCK_CENTER;
    else
      pane = &dock_container_->central_pane();*/

  } else {
    if (!pane)
      pane = &dock_container_->central_pane();
  }

  dock_container_->AddView(*pane, side, *view.view(), view.GetWindowTitle(),
                           gfx::Image() /*view.image()*/, percent_size);

  added_views_.emplace_back(&view);
}

void ViewManagerViews::CloseView(OpenedView& view) {
  if (view.view())
    dock_container_->RemoveView(*view.view());
  DestroyView(view);
}

void ViewManagerViews::OnFocusChanged(views::View* focused_before,
                                      views::View* focused_now) {
  if (opening_layout_)
    return;
  SetActiveView(GetActiveView());
}

void ViewManagerViews::OnViewClosed(views::View& view) {
  OpenedView* v = FindViewByViewsView(&view);
  if (v)
    CloseView(*v);
}

OpenedView* ViewManagerViews::GetActiveView() {
  return FindViewByViewsView(focus_manager_->GetFocusedView());
}

void ViewManagerViews::SplitView(OpenedView& view, bool vertically) {
  assert(false);
}
