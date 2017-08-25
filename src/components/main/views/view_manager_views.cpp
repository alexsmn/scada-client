#include "client/components/main/views/view_manager_views.h"

#include "base/auto_reset.h"
#include "client/components/main/opened_view.h"
#include "client/services/page.h"
#include "client/components/main/view_manager_delegate.h"
#include "client/window_info.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/root_view.h"
#include "ui/views/controls/split_view.h"

ViewManagerViews::ViewManagerViews(views::View& parent_view, ViewManagerDelegate* delegate)
    : ViewManager(delegate) {
  dock_container_.reset(new views::MultiSplitView);
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

void ViewManagerViews::SetViewTitle(OpenedView& view, const base::string16& title) {
  if (view.view())
    dock_container_->SetViewTitle(*view.view(), title);  
}

void ViewManagerViews::OpenLayout(Page& page, const PageLayout& layout) {
  {
    base::AutoReset<bool> opening_layout(&opening_layout_, true);

    for (int i = 0; i < page.GetWindowCount(); ++i) {
      WindowDefinition& win = page.GetWindow(i);
   
      // create window
      if (win.visible)
        CreateView(win);
    }

    OpenLayoutBlock(layout.main, dock_container_->root_pane());

    // Open windows not opended by layout.
    for (Views::iterator i = views_.begin(); i != views_.end(); ++i) {
      OpenedView& view = *i->view;
      if (view.view() && !view.view()->parent())
        AddView(view);
    }
  }

  OpenedView* view = FindViewByViewsView(focus_manager_->GetFocusedView());
  SetActiveView(view);
}

void ViewManagerViews::OpenLayoutBlock(const PageLayoutBlock& block, views::MultiSplitPane& pane) {
  if (block.type == PageLayoutBlock::SPLIT) {
    views::ViewSide dock_side = block.horz ? views::DOCK_BOTTOM :
                                             views::DOCK_RIGHT;
    int pane_percent_size = block.pos > 0 ? 100 - block.pos : 50;
    views::MultiSplitPane& new_pane =
        dock_container_->SplitPane(pane, dock_side, pane_percent_size);

    OpenLayoutBlock(*block.left, pane);
    OpenLayoutBlock(*block.right, new_pane);

  } else if (block.type == PageLayoutBlock::PANE) {
    for (size_t i = 0; i < block.wins.size(); ++i) {
      OpenedView* view = FindViewByID(block.wins[i]);
      if (view && !view->view()->parent()) {
        base::string16 title = view->GetWindowTitle();
        dock_container_->AddView(pane, views::DOCK_CENTER, *view->view(),
                                 title, gfx::Image() /*view->image()*/, 0);
      }
    }

    if (block.central)
      dock_container_->SetCentralPane(pane);
  }
}

void ViewManagerViews::SaveLayout(PageLayout& layout) {
  SaveLayoutBlock(layout.main, dock_container_->root_pane());
}

void ViewManagerViews::SaveLayoutBlock(PageLayoutBlock& block, views::MultiSplitPane& pane) {
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
      OpenedView* view = FindViewByViewsView(&tab_view.GetTabView(i));
      if (view)
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
  for ( ; view; view = view->parent()) {
    // Check this view is top level view.
    for (Views::const_iterator i = views_.begin(); i != views_.end(); ++i) {
      OpenedView* v = i->view;
      if (v->view() == view)
        return v;
    }
  }
  return NULL;
}

OpenedView* ViewManagerViews::FindFirstDataView(views::MultiSplitPane& from_pane) {
  if (from_pane.is_split()) {
    views::MultiSplitPane* c1 = from_pane.split_pane(0);
    views::MultiSplitPane* c2 = from_pane.split_pane(1);    
    OpenedView* v;
    if (c1 && (v = FindFirstDataView(*c1)))
      return v;
    if (c2 && (v = FindFirstDataView(*c2)))
      return v;
    return NULL;

  } else {
    views::TabView& tab_view = from_pane.tab_view();
    for (int i = 0; i < tab_view.tab_count(); ++i) {
      OpenedView* v = FindViewByViewsView(&tab_view.GetTabView(i));
      if (v && !v->window_info().is_pane())
        return v;
    }
    return NULL;
  }
}

void ViewManagerViews::OnShowViewTabContextMenu(views::View& view, gfx::Point point) {
  OpenedView* v = FindViewByViewsView(&view);
  if (!v)
    return;

  ActivateView(*v);

  delegate_->OnShowTabPopupMenu(*v, point);
}

void ViewManagerViews::AddView(OpenedView& view) {
  assert(view.view());

  views::MultiSplitPane* pane = NULL;
  views::ViewSide side = views::DOCK_CENTER;
  int percent_size = 50;
  
  if (!pane && view.window_info().is_pane()) {
    percent_size = view.window_info().dock_bottom() ?
        view.window_info().cy * 100 / 768:
        view.window_info().cx * 100 / 1024;
    side = view.window_info().dock_bottom() ? views::DOCK_BOTTOM :
                                              views::DOCK_LEFT;
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
}

void ViewManagerViews::CloseView(OpenedView& view) {
  if (view.view())
    dock_container_->RemoveView(*view.view());
  DestroyView(view);
}

void ViewManagerViews::OnFocusChanged(views::View* focused_before, views::View* focused_now) {
  if (opening_layout_)
    return;
  OpenedView* view = FindViewByViewsView(focused_now);
  SetActiveView(view);
}

void ViewManagerViews::OnViewClosed(views::View& view) {
  OpenedView* v = FindViewByViewsView(&view);
  if (v)
    CloseView(*v);
}
