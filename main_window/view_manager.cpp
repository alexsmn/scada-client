#include "main_window/view_manager.h"

#include "base/auto_reset.h"
#include "base/boost_log.h"
#include "base/check.h"
#include "controller/window_info.h"
#include "main_window/opened_view/opened_view.h"
#include "main_window/view_manager_delegate.h"
#include "profile/page.h"
#include "profile/window_definition.h"
#include "resources/common_resources.h"

#include <algorithm>

#if defined(UI_QT)
ViewManager::ViewManager(QMainWindow& main_window, ViewManagerDelegate& delegate)
    : delegate_{delegate},
      current_page_{std::make_unique<Page>()},
      component_{main_window} {
  component_.SetCloseViewHandler([this](scada::aui::ViewManagerViewId view_id) {
    if (auto* view = FindViewByComponentId(view_id))
      CloseView(*view);
  });

  component_.SetActiveViewChangedHandler(
      [this](std::optional<scada::aui::ViewManagerViewId> view_id) {
        SetActiveView(view_id ? FindViewByComponentId(*view_id) : nullptr);
      });

  component_.SetTabPopupMenuHandler(
      [this](scada::aui::ViewManagerViewId view_id,
             const scada::aui::Point& point) {
        if (auto* view = FindViewByComponentId(view_id))
          delegate_.OnShowTabPopupMenu(*view, point);
      });
}
#endif

ViewManager::~ViewManager() {
  // Page must be closed before destruction, as closing calls delegate.
  scada::base::Check(views_.empty());
}

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

  // Keep the cached active view in sync deterministically. The platform
  // component normally reports activation back through its active-view-changed
  // handler, but that signal does not fire when the windows are hidden (e.g.
  // during headless E2E runs), which would otherwise leave GetActiveView() —
  // and anything derived from it, such as the context menu — stale. A
  // redundant call here is a no-op (SetActiveView early-returns on no change).
  SetActiveView(const_cast<OpenedView*>(&view));
}

void ViewManager::CloseView(OpenedView& view) {
  component_.RemoveView(GetComponentViewId(view));
  DestroyView(view);
}

void ViewManager::SplitView(OpenedView& view, bool vertically) {
  component_.SplitView(GetComponentViewId(view), vertically);
}

void ViewManager::OpenLayout(Page& page, const PageLayout& layout) {
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

scada::aui::ViewManagerViewId ViewManager::GetComponentViewId(
    const OpenedView& view) const {
  return reinterpret_cast<scada::aui::ViewManagerViewId>(&view);
}

scada::aui::ViewManagerViewInfo ViewManager::GetComponentViewInfo(
    OpenedView& view) const {
  const auto& window_info = view.window_info();
  return scada::aui::ViewManagerViewInfo{
      .id = GetComponentViewId(view),
      .widget = view.view(),
      .title = view.GetWindowTitle(),
#if defined(UI_QT)
      .state_name = "dock-" + std::to_string(view.window_id()),
#endif
      .dock = window_info.is_pane(),
      .dock_bottom = window_info.dock_bottom(),
      // Subsystems is the one pane that never tabifies onto its neighbours.
      // Under the reshell this is a no-op — the activity rail's Devices mode
      // shows Subsystems alone, so there is nothing to tab onto. It still
      // matters for the legacy default page (pages/initial_page.cpp), where
      // Objects+Portfolio and Subsystems form two stacked left columns
      // precisely because this pane refuses to join their tab bar. Generalize
      // to a WIN_NO_TABIFY flag if a second such pane ever appears.
      .tabify_existing_dock = window_info.command_id != ID_HARDWARE_VIEW};
}

OpenedView* ViewManager::FindViewByComponentId(
    scada::aui::ViewManagerViewId view_id) const {
  for (auto* view : views_) {
    if (GetComponentViewId(*view) == view_id) {
      return view;
    }
  }
  return nullptr;
}

std::vector<scada::aui::ViewManagerViewInfo>
ViewManager::GetComponentViewInfos() const {
  std::vector<scada::aui::ViewManagerViewInfo> result;
  result.reserve(views_.size());
  for (auto* view : views_) {
    result.emplace_back(GetComponentViewInfo(*view));
  }
  return result;
}

scada::aui::ViewManagerSavedLayout ViewManager::ToComponentLayout(
    const PageLayout& layout) const {
  scada::aui::ViewManagerSavedLayout component_layout;
  component_layout.main = ToComponentLayoutNode(layout.main);
#if defined(UI_QT)
  component_layout.dock_state_blob = layout.blob;
#endif
  return component_layout;
}

scada::aui::ViewManagerLayoutNode ViewManager::ToComponentLayoutNode(
    const PageLayoutBlock& block) const {
  scada::aui::ViewManagerLayoutNode component_block;
  if (block.type == PageLayoutBlock::PANE) {
    component_block.type = scada::aui::ViewManagerLayoutNode::Type::Tabs;
    for (int window_id : block.wins) {
      if (auto* view = FindViewByID(window_id)) {
        component_block.tabs.emplace_back(GetComponentViewId(*view));
      }
    }
    return component_block;
  }

  component_block.type = scada::aui::ViewManagerLayoutNode::Type::Split;
  component_block.split_vertical = block.horz;
  component_block.split_pos = block.pos;
  component_block.left = std::make_unique<scada::aui::ViewManagerLayoutNode>(
      ToComponentLayoutNode(*block.left));
  component_block.right = std::make_unique<scada::aui::ViewManagerLayoutNode>(
      ToComponentLayoutNode(*block.right));
  return component_block;
}

void ViewManager::FromComponentLayout(
    const scada::aui::ViewManagerSavedLayout& component_layout,
    PageLayout& layout) const {
  FromComponentLayoutNode(component_layout.main, layout.main);
#if defined(UI_QT)
  layout.blob = component_layout.dock_state_blob;
#endif
}

void ViewManager::FromComponentLayoutNode(
    const scada::aui::ViewManagerLayoutNode& component_block,
    PageLayoutBlock& block) const {
  if (component_block.type == scada::aui::ViewManagerLayoutNode::Type::Tabs) {
    for (scada::aui::ViewManagerViewId view_id : component_block.tabs) {
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

  scada::base::Check(std::ranges::find(views_, &view) != views_.end());
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
  } catch (const std::exception& e) {
    // Log it: this is the one place a view can fail to open with no trace at
    // all. `OpenedView::Init` throws for both "no controller for this command"
    // (unregistered type, or an admin gate the session does not pass) and "the
    // controller built no widget", and swallowing that silently leaves a page
    // simply missing a window. The screenshot generator's "Window type not
    // found" failures were this, invisible for want of one line.
    BOOST_LOG_TRIVIAL(error)
        << "Failed to create view " << def.type << ": " << e.what();
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
    scada::base::AutoReset<bool> opening_layout(&opening_layout_, true);

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
  scada::base::Check(!closing_page_);

  // Prevent WindowDefinition delete on close child windows.
  scada::base::AutoReset<bool> closing_page(&closing_page_, true);

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
        scada::base::Check(!win.visible);
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
