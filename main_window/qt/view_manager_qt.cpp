#include "main_window/qt/view_manager_qt.h"

#include "resources/common_resources.h"
#include "controller/window_info.h"
#include "main_window/opened_view.h"
#include "main_window/view_manager_delegate.h"
#include "profile/page.h"

#include <QMainWindow>

#include <cstdint>
#include <ranges>
#include <string>

ViewManagerQt::ViewManagerQt(QMainWindow& main_window,
                             ViewManagerDelegate& delegate)
    : ViewManager{delegate}, component_{main_window} {
  component_.SetCloseViewHandler([this](ViewManagerQtComponent::ViewId view_id) {
    if (auto* view = FindViewByComponentId(view_id))
      CloseView(*view);
  });

  component_.SetActiveViewChangedHandler(
      [this](std::optional<ViewManagerQtComponent::ViewId> view_id) {
        SetActiveView(view_id ? FindViewByComponentId(*view_id) : nullptr);
      });

  component_.SetTabPopupMenuHandler(
      [this](ViewManagerQtComponent::ViewId view_id, const QPoint& point) {
        if (auto* view = FindViewByComponentId(view_id))
          delegate_.OnShowTabPopupMenu(*view, point);
      });
}

ViewManagerQt::~ViewManagerQt() {}

OpenedView* ViewManagerQt::GetActiveView() {
  auto view_id = component_.GetActiveViewId();
  return view_id ? FindViewByComponentId(*view_id) : nullptr;
}

void ViewManagerQt::SetViewTitle(OpenedView& view,
                                 const std::u16string& title) {
  component_.SetViewTitle(GetComponentViewId(view), title);
}

void ViewManagerQt::ActivateView(const OpenedView& view) {
  component_.ActivateView(GetComponentViewId(view));
}

void ViewManagerQt::CloseView(OpenedView& view) {
  component_.RemoveView(GetComponentViewId(view));
  DestroyView(view);
}

void ViewManagerQt::SplitView(OpenedView& view, bool vertically) {
  component_.SplitView(GetComponentViewId(view), vertically);
}

void ViewManagerQt::OpenLayout(Page& page, const PageLayout& layout) {
  auto views = GetComponentViewInfos();
  auto component_layout = ToComponentLayout(layout);
  component_.OpenLayout(views, component_layout);
}

void ViewManagerQt::SaveLayout(PageLayout& layout) {
  auto views = GetComponentViewInfos();
  FromComponentLayout(component_.SaveLayout(views), layout);
}

void ViewManagerQt::AddView(OpenedView& view) {
  component_.AddView(
      GetComponentViewInfo(view),
      active_view_ ? std::optional{GetComponentViewId(*active_view_)}
                   : std::nullopt);
}

ViewManagerQt::ComponentViewInfo ViewManagerQt::GetComponentViewInfo(
    OpenedView& view) const {
  const auto& window_info = view.window_info();
  return ComponentViewInfo{
      .id = GetComponentViewId(view),
      .widget = view.view(),
      .title = view.GetWindowTitle(),
      .state_name = "dock-" + std::to_string(view.window_id()),
      .dock = window_info.is_pane(),
      .dock_bottom = window_info.dock_bottom(),
      .tabify_existing_dock = window_info.command_id != ID_HARDWARE_VIEW};
}

std::vector<ViewManagerQt::ComponentViewInfo>
ViewManagerQt::GetComponentViewInfos() const {
  std::vector<ComponentViewInfo> result;
  result.reserve(views_.size());
  for (auto* view : views_)
    result.emplace_back(GetComponentViewInfo(*view));
  return result;
}

ViewManagerQtComponent::ViewId ViewManagerQt::GetComponentViewId(
    const OpenedView& view) const {
  return reinterpret_cast<ViewManagerQtComponent::ViewId>(&view);
}

OpenedView* ViewManagerQt::FindViewByComponentId(
    ViewManagerQtComponent::ViewId view_id) const {
  auto i = std::ranges::find_if(views_, [this, view_id](const OpenedView* view) {
    return GetComponentViewId(*view) == view_id;
  });
  return i != views_.end() ? *i : nullptr;
}

ViewManagerQt::ComponentSavedLayout ViewManagerQt::ToComponentLayout(
    const PageLayout& layout) const {
  ComponentSavedLayout component_layout;
  component_layout.main = ToComponentLayoutNode(layout.main);
  component_layout.dock_state_blob = layout.blob;
  return component_layout;
}

ViewManagerQt::ComponentLayoutNode ViewManagerQt::ToComponentLayoutNode(
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

void ViewManagerQt::FromComponentLayout(
    const ComponentSavedLayout& component_layout,
    PageLayout& layout) const {
  FromComponentLayoutNode(component_layout.main, layout.main);
  layout.blob = component_layout.dock_state_blob;
}

void ViewManagerQt::FromComponentLayoutNode(
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
