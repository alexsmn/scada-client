#include "main_window/wt/view_manager_wt.h"

#include "resources/common_resources.h"
#include "controller/window_info.h"
#include "main_window/opened_view.h"
#include "main_window/view_manager_delegate.h"
#include "profile/page.h"

#include <wt/WApplication.h>

#include <cstdint>
#include <ranges>
#include <string>

ViewManagerWt::ViewManagerWt(ViewManagerDelegate& delegate)
    : ViewManager{delegate} {
  component_.SetCloseViewHandler([this](ViewManagerWtComponent::ViewId view_id) {
    if (auto* view = FindViewByComponentId(view_id))
      CloseView(*view);
  });
}

ViewManagerWt::~ViewManagerWt() = default;

Wt::WLayout& ViewManagerWt::root_layout() {
  return component_.root_layout();
}

OpenedView* ViewManagerWt::GetActiveView() {
  auto view_id = component_.GetActiveViewId();
  return view_id ? FindViewByComponentId(*view_id) : nullptr;
}

void ViewManagerWt::SetViewTitle(OpenedView& view,
                                 const std::u16string& title) {
  component_.SetViewTitle(GetComponentViewId(view), title);
}

void ViewManagerWt::ActivateView(const OpenedView& view) {
  component_.ActivateView(GetComponentViewId(view));
}

void ViewManagerWt::CloseView(OpenedView& view) {
  if (component_.RemoveView(GetComponentViewId(view)))
    view.ReleaseView();
  DestroyView(view);
}

void ViewManagerWt::SplitView(OpenedView& view, bool vertically) {
  component_.SplitView(GetComponentViewId(view), vertically);
}

void ViewManagerWt::OpenLayout(Page& page, const PageLayout& layout) {
  Wt::WApplication::UpdateLock update_lock{Wt::WApplication::instance()};

  auto views = GetComponentViewInfos();
  component_.OpenLayout(views, ToComponentLayout(layout));
}

void ViewManagerWt::SaveLayout(PageLayout& layout) {
  auto views = GetComponentViewInfos();
  FromComponentLayout(component_.SaveLayout(views), layout);
}

void ViewManagerWt::AddView(OpenedView& view) {
  component_.AddView(
      GetComponentViewInfo(view),
      active_view_ ? std::optional{GetComponentViewId(*active_view_)}
                   : std::nullopt);
}

ViewManagerWt::ComponentViewInfo ViewManagerWt::GetComponentViewInfo(
    OpenedView& view) const {
  const auto& window_info = view.window_info();
  return ComponentViewInfo{
      .id = GetComponentViewId(view),
      .widget = view.view(),
      .title = view.GetWindowTitle(),
      .dock = window_info.is_pane(),
      .dock_bottom = window_info.dock_bottom(),
      .tabify_existing_dock = window_info.command_id != ID_HARDWARE_VIEW};
}

std::vector<ViewManagerWt::ComponentViewInfo>
ViewManagerWt::GetComponentViewInfos() const {
  std::vector<ComponentViewInfo> result;
  result.reserve(views_.size());
  for (auto* view : views_)
    result.emplace_back(GetComponentViewInfo(*view));
  return result;
}

ViewManagerWtComponent::ViewId ViewManagerWt::GetComponentViewId(
    const OpenedView& view) const {
  return reinterpret_cast<ViewManagerWtComponent::ViewId>(&view);
}

OpenedView* ViewManagerWt::FindViewByComponentId(
    ViewManagerWtComponent::ViewId view_id) const {
  auto i = std::ranges::find_if(views_, [this, view_id](const OpenedView* view) {
    return GetComponentViewId(*view) == view_id;
  });
  return i != views_.end() ? *i : nullptr;
}

ViewManagerWt::ComponentSavedLayout ViewManagerWt::ToComponentLayout(
    const PageLayout& layout) const {
  ComponentSavedLayout component_layout;
  component_layout.main = ToComponentLayoutNode(layout.main);
  return component_layout;
}

ViewManagerWt::ComponentLayoutNode ViewManagerWt::ToComponentLayoutNode(
    const PageLayoutBlock& block) const {
  ComponentLayoutNode component_block;
  if (block.type == PageLayoutBlock::PANE) {
    component_block.type = ComponentLayoutNode::Type::Tabs;
    for (int window_id : block.wins) {
      if (auto* view = FindViewByID(window_id))
        component_block.tabs.emplace_back(GetComponentViewId(*view));
    }
    return component_block;
  }

  component_block.type = ComponentLayoutNode::Type::Split;
  component_block.split_vertical = block.horz;
  component_block.split_pos = block.pos;
  component_block.left =
      std::make_unique<ComponentLayoutNode>(ToComponentLayoutNode(*block.left));
  component_block.right =
      std::make_unique<ComponentLayoutNode>(ToComponentLayoutNode(*block.right));
  return component_block;
}

void ViewManagerWt::FromComponentLayout(
    const ComponentSavedLayout& component_layout,
    PageLayout& layout) const {
  FromComponentLayoutNode(component_layout.main, layout.main);
}

void ViewManagerWt::FromComponentLayoutNode(
    const ComponentLayoutNode& component_block,
    PageLayoutBlock& block) const {
  if (component_block.type == ComponentLayoutNode::Type::Tabs) {
    for (auto view_id : component_block.tabs) {
      if (auto* view = FindViewByComponentId(view_id))
        block.add(view->window_id());
    }
    return;
  }

  block.split(component_block.split_vertical);
  block.pos = component_block.split_pos;
  if (component_block.left)
    FromComponentLayoutNode(*component_block.left, *block.left);
  if (component_block.right)
    FromComponentLayoutNode(*component_block.right, *block.right);
}
