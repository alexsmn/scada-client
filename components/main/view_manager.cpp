#include "components/main/view_manager.h"

#include "base/auto_reset.h"
#include "components/main/opened_view.h"
#include "components/main/view_manager_delegate.h"
#include "services/page.h"
#include "window_definition.h"
#include "window_info.h"


#include <algorithm>

ViewManager::ViewManager(ViewManagerDelegate& delegate)
    : current_page_{std::make_unique<Page>()}, delegate_{delegate} {}

ViewManager::~ViewManager() {
  // Page must be closed before destruction, as closing calls delegate.
  assert(views_.empty());
}

OpenedView* ViewManager::FindViewByID(int id) const {
  for (auto& view_info : views_) {
    if (view_info.view->window_id() == id)
      return view_info.view;
  }
  return nullptr;
}

OpenedView* ViewManager::FindViewByType(unsigned type) const {
  for (auto& view_info : views_) {
    if (view_info.view->window_info().command_id == type)
      return view_info.view;
  }
  return nullptr;
}

void ViewManager::SetActiveView(OpenedView* view) {
  if (active_view_ == view)
    return;

  active_view_ = view;

  delegate_.OnActiveViewChanged(view);
}

void ViewManager::DestroyView(OpenedView& view) {
  if (&view == active_view_)
    SetActiveView(nullptr);

  auto p = std::find_if(
      views_.begin(), views_.end(),
      [&view](ViewInfo& view_info) { return view_info.view == &view; });
  assert(p != views_.end());
  WindowDefinition* definition = p->definition;
  views_.erase(p);

  if (!is_closing_page())
    delegate_->OnViewClosed(view, *definition);

  delete &view;
}

OpenedView* ViewManager::CreateView(WindowDefinition& def,
                                    OpenedView* after_view) {
  std::unique_ptr<OpenedView> view;
  try {
    view = delegate_.OnCreateView(def);
  } catch (const std::exception&) {
    return nullptr;
  }

  ViewInfo view_info{view.release(), &def};
  views_.push_back(view_info);

  // TODO: Process |after_view|.

  if (!opening_layout_)
    AddView(*view_info.view);

  return view_info.view;
}

void ViewManager::OpenPage(const Page& page) {
  ClosePage();

  *current_page_ = page;

  OpenLayout(*current_page_, current_page_->layout());
}

void ViewManager::SavePage() {
  for (auto& view_info : views_)
    view_info.view->Save(*view_info.definition);

  PageLayout& layout = current_page_->layout();
  layout.Clear();
  SaveLayout(layout);
}

void ViewManager::ClosePage() {
  assert(!closing_page_);

  // Prevent WindowDefinition delete on close child windows.
  base::AutoReset<bool> closing_page(&closing_page_, true);

  while (!views_.empty())
    CloseView(*views_.front().view);
}

OpenedView* ViewManager::OpenView(const WindowDefinition& def,
                                  bool make_active,
                                  OpenedView* after_view) {
  const WindowInfo& info = def.window_info();

  WindowDefinition* window_def = nullptr;

  if (info.is_pane()) {
    OpenedView* view = FindViewByType(info.command_id);
    if (view) {
      if (make_active)
        ActivateView(*view);
      return view;
    }

    // If window is not found try to find stored invisible definition for
    // this window type.
    Page& page = current_page();
    for (int i = 0; i < page.GetWindowCount(); ++i) {
      WindowDefinition& win = page.GetWindow(i);
      if (&win.window_info() == &info) {
        assert(!win.visible);
        win.visible = true;
        window_def = &win;
        break;
      }
    }
  }

  LOG(INFO) << "Open window " << info.title;

  if (!window_def)
    window_def = &current_page().AddWindow(def);

  // add win

  OpenedView* view = CreateView(*window_def, after_view);
  if (!view)
    return nullptr;

  view->SetModified(true);

  if (make_active)
    ActivateView(*view);

  return view;
}
