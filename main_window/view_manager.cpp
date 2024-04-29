#include "main_window/view_manager.h"

#include "base/auto_reset.h"
#include "base/containers/contains.h"
#include "controller/window_info.h"
#include "main_window/opened_view.h"
#include "main_window/view_manager_delegate.h"
#include "profile/page.h"
#include "profile/window_definition.h"

#include <algorithm>

ViewManager::ViewManager(ViewManagerDelegate& delegate)
    : current_page_{std::make_unique<Page>()}, delegate_{delegate} {}

ViewManager::~ViewManager() {
  // Page must be closed before destruction, as closing calls delegate.
  assert(views_.empty());
}

OpenedView* ViewManager::FindViewByID(int id) const {
  auto i = std::ranges::find(
      views_, id, [](const OpenedView* view) { return view->window_id(); });
  return i == views_.end() ? nullptr : *i;
}

bool ViewManager::IsViewAdded(OpenedView& opened_view) const {
  return base::Contains(added_views_, &opened_view);
}

OpenedView* ViewManager::FindViewByType(std::string_view window_type) const {
  auto i = std::ranges::find(views_, window_type, [](const OpenedView* view) {
    return view->window_info().name;
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
  if (&view == active_view_) {
    SetActiveView(nullptr);
  }

  assert(base::Contains(views_, &view));
  std::erase(views_, &view);
  std::erase(added_views_, &view);

  delegate_.OnViewClosed(view);

  delete &view;
}

OpenedView* ViewManager::CreateView(WindowDefinition& def,
                                    const OpenedView* after_view) {
  std::unique_ptr<OpenedView> opened_view;
  try {
    opened_view = delegate_.OnCreateView(def);
  } catch (const std::exception&) {
    return nullptr;
  }

  if (!opened_view) {
    return nullptr;
  }

  auto& opened_view_ref = *views_.emplace_back(opened_view.release());

  // TODO: Process |after_view|.

  if (!opening_layout_) {
    AddView(opened_view_ref);
  }

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
      if (win.visible) {
        CreateView(win);
      }
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

  while (!views_.empty()) {
    CloseView(*views_.front());
  }
}

OpenedView* ViewManager::OpenView(const WindowDefinition& def,
                                  bool activate,
                                  const OpenedView* after_view) {
  const auto* window_info = FindWindowInfoByName(def.type);
  if (!window_info) {
    LOG(ERROR) << "Window type not found: " << def.type;
    return nullptr;
  }

  WindowDefinition* window_def = nullptr;

  if (window_info->is_pane()) {
    if (auto* opened_view = FindViewByType(window_info->name)) {
      if (activate) {
        ActivateView(*opened_view);
      }
      return opened_view;
    }

    // If window is not found try to find stored invisible definition for
    // this window type.
    const Page& page = current_page();
    for (int i = 0; i < page.GetWindowCount(); ++i) {
      WindowDefinition& win = page.GetWindow(i);
      if (win.type == def.type) {
        assert(!win.visible);
        win.visible = true;
        window_def = &win;
        break;
      }
    }
  }

  LOG(INFO) << "Open window " << std::u16string{window_info->title};

  if (!window_def) {
    window_def = &current_page().AddWindow(def);
  }

  // add win

  auto* opened_view = CreateView(*window_def, after_view);
  if (!opened_view) {
    return nullptr;
  }

  opened_view->SetModified(true);

  if (activate) {
    ActivateView(*opened_view);
  }

  return opened_view;
}
