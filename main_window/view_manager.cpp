#include "main_window/view_manager.h"

#include "base/boost_log.h"
#include "base/auto_reset.h"
#include "controller/window_info.h"
#include "main_window/opened_view.h"
#include "main_window/view_manager_delegate.h"
#include "profile/page.h"
#include "profile/window_definition.h"
#include "resources/common_resources.h"

#if defined(UI_WT)
#include <wt/WApplication.h>
#endif

#include <algorithm>

#if defined(UI_QT)
ViewManager::ViewManager(QMainWindow& main_window, ViewManagerDelegate& delegate)
    : delegate_{delegate},
      current_page_{std::make_unique<Page>()},
      component_{main_window} {
  component_.SetCloseViewHandler([this](aui::ViewManagerViewId view_id) {
    if (auto* view = FindViewByComponentId(view_id))
      CloseView(*view);
  });

  component_.SetActiveViewChangedHandler(
      [this](std::optional<aui::ViewManagerViewId> view_id) {
        SetActiveView(view_id ? FindViewByComponentId(*view_id) : nullptr);
      });

  component_.SetTabPopupMenuHandler(
      [this](aui::ViewManagerViewId view_id, const aui::Point& point) {
        if (auto* view = FindViewByComponentId(view_id))
          delegate_.OnShowTabPopupMenu(*view, point);
      });
}
#elif defined(UI_WT)
ViewManager::ViewManager(ViewManagerDelegate& delegate)
    : delegate_{delegate}, current_page_{std::make_unique<Page>()} {
  component_.SetCloseViewHandler([this](aui::ViewManagerViewId view_id) {
    if (auto* view = FindViewByComponentId(view_id))
      CloseView(*view);
  });
}
#endif

ViewManager::~ViewManager() {
  // Page must be closed before destruction, as closing calls delegate.
  assert(views_.empty());
}

#if defined(UI_WT)
Wt::WLayout& ViewManager::root_layout() {
  return component_.root_layout();
}
#endif

OpenedView* ViewManager::GetActiveView() {
  auto view_id = component_.GetActiveViewId();
  return view_id ? FindViewByComponentId(*view_id) : nullptr;
}

void ViewManager::SetViewTitle(OpenedView& view,
                               const std::u16string& title) {
  component_.SetViewTitle(GetComponentViewId(view), title);
}

void ViewManager::ActivateView(const OpenedView& view) {
  component_.ActivateView(GetComponentViewId(view));
}

void ViewManager::CloseView(OpenedView& view) {
  if (component_.RemoveView(GetComponentViewId(view))) {
#if defined(UI_WT)
    view.ReleaseView();
#endif
  }
  DestroyView(view);
}

void ViewManager::SplitView(OpenedView& view, bool vertically) {
  component_.SplitView(GetComponentViewId(view), vertically);
}

void ViewManager::OpenLayout(Page& page, const PageLayout& layout) {
#if defined(UI_WT)
  Wt::WApplication::UpdateLock update_lock{Wt::WApplication::instance()};
#endif

  auto views = GetComponentViewInfos();
  auto component_layout = ToComponentLayout(layout);
  component_.OpenLayout(views, component_layout);
}

void ViewManager::SaveLayout(PageLayout& layout) {
  auto views = GetComponentViewInfos();
  FromComponentLayout(component_.SaveLayout(views), layout);
}

void ViewManager::AddView(OpenedView& view) {
  component_.AddView(
      GetComponentViewInfo(view),
      active_view_ ? std::optional{
                         GetComponentViewId(*active_view_)}
                   : std::nullopt);
}

aui::ViewManagerViewId ViewManager::GetComponentViewId(
    const OpenedView& view) const {
  return reinterpret_cast<aui::ViewManagerViewId>(&view);
}

aui::ViewManagerViewInfo ViewManager::GetComponentViewInfo(
    OpenedView& view) const {
  const auto& window_info = view.window_info();
  return aui::ViewManagerViewInfo{
      .id = GetComponentViewId(view),
      .widget = view.view(),
      .title = view.GetWindowTitle(),
#if defined(UI_QT)
      .state_name = "dock-" + std::to_string(view.window_id()),
#endif
      .dock = window_info.is_pane(),
      .dock_bottom = window_info.dock_bottom(),
      .tabify_existing_dock = window_info.command_id != ID_HARDWARE_VIEW};
}

OpenedView* ViewManager::FindViewByComponentId(
    aui::ViewManagerViewId view_id) const {
  for (auto* view : views_) {
    if (GetComponentViewId(*view) == view_id) {
      return view;
    }
  }
  return nullptr;
}

std::vector<aui::ViewManagerViewInfo> ViewManager::GetComponentViewInfos()
    const {
  std::vector<aui::ViewManagerViewInfo> result;
  result.reserve(views_.size());
  for (auto* view : views_) {
    result.emplace_back(GetComponentViewInfo(*view));
  }
  return result;
}

aui::ViewManagerSavedLayout ViewManager::ToComponentLayout(
    const PageLayout& layout) const {
  aui::ViewManagerSavedLayout component_layout;
  component_layout.main = ToComponentLayoutNode(layout.main);
  if constexpr (requires { component_layout.dock_state_blob; }) {
    component_layout.dock_state_blob = layout.blob;
  }
  return component_layout;
}

aui::ViewManagerLayoutNode ViewManager::ToComponentLayoutNode(
    const PageLayoutBlock& block) const {
  aui::ViewManagerLayoutNode component_block;
  if (block.type == PageLayoutBlock::PANE) {
    component_block.type = aui::ViewManagerLayoutNode::Type::Tabs;
    for (int window_id : block.wins) {
      if (auto* view = FindViewByID(window_id)) {
        component_block.tabs.emplace_back(GetComponentViewId(*view));
      }
    }
    return component_block;
  }

  component_block.type = aui::ViewManagerLayoutNode::Type::Split;
  component_block.split_vertical = block.horz;
  component_block.split_pos = block.pos;
  component_block.left = std::make_unique<aui::ViewManagerLayoutNode>(
      ToComponentLayoutNode(*block.left));
  component_block.right = std::make_unique<aui::ViewManagerLayoutNode>(
      ToComponentLayoutNode(*block.right));
  return component_block;
}

void ViewManager::FromComponentLayout(
    const aui::ViewManagerSavedLayout& component_layout,
    PageLayout& layout) const {
  FromComponentLayoutNode(component_layout.main, layout.main);
  if constexpr (requires { component_layout.dock_state_blob; }) {
    layout.blob = component_layout.dock_state_blob;
  }
}

void ViewManager::FromComponentLayoutNode(
    const aui::ViewManagerLayoutNode& component_block,
    PageLayoutBlock& block) const {
  if (component_block.type == aui::ViewManagerLayoutNode::Type::Tabs) {
    for (aui::ViewManagerViewId view_id : component_block.tabs) {
      if (auto* view = FindViewByComponentId(view_id)) {
        block.add(view->window_id());
      }
    }
    return;
  }

  block.split(component_block.split_vertical);
  block.pos = component_block.split_pos;
  if (component_block.left) {
    FromComponentLayoutNode(*component_block.left, *block.left);
  }
  if (component_block.right) {
    FromComponentLayoutNode(*component_block.right, *block.right);
  }
}

OpenedView* ViewManager::FindViewByID(int id) const {
  auto i = std::ranges::find(
      views_, id, [](const OpenedView* view) { return view->window_id(); });
  return i == views_.end() ? nullptr : *i;
}

bool ViewManager::IsViewAdded(OpenedView& opened_view) const {
  return std::ranges::find(added_views_, &opened_view) != added_views_.end();
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

  assert(std::ranges::find(views_, &view) != views_.end());
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
    BOOST_LOG_TRIVIAL(error) << "Window type not found: " << def.type;
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

  BOOST_LOG_TRIVIAL(info) << "Open window " << std::u16string{window_info->title};

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
