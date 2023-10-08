#include "components/main/view_manager.h"

#include "base/auto_reset.h"
#include "components/main/opened_view.h"
#include "components/main/view_manager_delegate.h"
#include "services/page.h"
#include "controller/window_definition.h"
#include "controller/window_info.h"

#include <algorithm>

ViewManager::ViewManager(ViewManagerDelegate& delegate)
    : current_page_{std::make_unique<Page>()}, delegate_{delegate} {}

ViewManager::~ViewManager() {
  // Page must be closed before destruction, as closing calls delegate.
  assert(views_.empty());
}

OpenedView* ViewManager::FindViewByID(int id) const {
  auto i = std::find_if(
      views_.begin(), views_.end(),
      [id](OpenedView* opened_view) { return opened_view->window_id() == id; });
  return i == views_.end() ? nullptr : *i;
}

bool ViewManager::IsViewAdded(OpenedView& opened_view) const {
  auto i = std::find(added_views_.begin(), added_views_.end(), &opened_view);
  return i != added_views_.end();
}

OpenedView* ViewManager::FindViewByType(const WindowInfo& window_info) const {
  auto i = std::find_if(views_.begin(), views_.end(),
                        [&window_info](OpenedView* opened_view) {
                          return &opened_view->window_info() == &window_info;
                        });
  return i == views_.end() ? nullptr : *i;
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

  {
    auto i = std::find(views_.begin(), views_.end(), &view);
    assert(i != views_.end());
    views_.erase(i);
  }

  {
    auto i = std::find(added_views_.begin(), added_views_.end(), &view);
    if (i != added_views_.end())
      added_views_.erase(i);
  }

  delegate_.OnViewClosed(view);

  delete &view;
}

OpenedView* ViewManager::CreateView(WindowDefinition& def,
                                    OpenedView* after_view) {
  std::unique_ptr<OpenedView> opened_view;
  try {
    opened_view = delegate_.OnCreateView(def);
  } catch (const std::exception&) {
    return nullptr;
  }

  if (!opened_view)
    return nullptr;

  auto& opened_view_ref = *views_.emplace_back(opened_view.release());

  // TODO: Process |after_view|.

  if (!opening_layout_)
    AddView(opened_view_ref);

  return &opened_view_ref;
}

void ViewManager::OpenPage(const Page& page) {
  ClosePage();

  *current_page_ = page;

  {
    // Do not call AddView() from CreateView() and don't process focus change.
    base::AutoReset<bool> opening_layout(&opening_layout_, true);

    for (int i = 0; i < current_page_->GetWindowCount(); ++i) {
      WindowDefinition& win = current_page_->GetWindow(i);

      // create window
      if (win.visible)
        CreateView(win, NULL);
    }

    OpenLayout(*current_page_, current_page_->layout);
  }

  SetActiveView(GetActiveView());
}

void ViewManager::SavePage() {
  for (auto* opened_view : views_)
    opened_view->Save();

  PageLayout& layout = current_page_->layout;
  layout.Clear();
  SaveLayout(layout);
}

void ViewManager::ClosePage() {
  assert(!closing_page_);

  // Prevent WindowDefinition delete on close child windows.
  base::AutoReset<bool> closing_page(&closing_page_, true);

  while (!views_.empty())
    CloseView(*views_.front());
}

OpenedView* ViewManager::OpenView(const WindowDefinition& def,
                                  bool make_active,
                                  OpenedView* after_view) {
  WindowDefinition* window_def = nullptr;

  const WindowInfo& window_info = def.window_info();
  if (window_info.is_pane()) {
    if (auto* opened_view = FindViewByType(window_info)) {
      if (make_active)
        ActivateView(*opened_view);
      return opened_view;
    }

    // If window is not found try to find stored invisible definition for
    // this window type.
    Page& page = current_page();
    for (int i = 0; i < page.GetWindowCount(); ++i) {
      WindowDefinition& win = page.GetWindow(i);
      if (&win.window_info() == &window_info) {
        assert(!win.visible);
        win.visible = true;
        window_def = &win;
        break;
      }
    }
  }

  LOG(INFO) << "Open window " << std::u16string{window_info.title};

  if (!window_def)
    window_def = &current_page().AddWindow(def);

  // add win

  auto* opened_view = CreateView(*window_def, after_view);
  if (!opened_view)
    return nullptr;

  opened_view->SetModified(true);

  if (make_active)
    ActivateView(*opened_view);

  return opened_view;
}
