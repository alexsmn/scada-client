#include "aui/wt/view_manager_wt_component.h"

#include <wt/WBorderLayout.h>
#include <wt/WContainerWidget.h>
#include <wt/WHBoxLayout.h>
#include <wt/WMenuItem.h>
#include <wt/WStackedWidget.h>
#include <wt/WString.h>
#include <wt/WTabWidget.h>
#include <wt/WVBoxLayout.h>
#include <wt/WWidgetItem.h>

#include <algorithm>
#include <cassert>
#include <ranges>

namespace {

const char sc_layoutWidgetTypeProp[] = "LayoutWidgetType";

enum class LayoutWidgetType { Splitter, Tabs, Count };

std::string ToString(LayoutWidgetType type) {
  constexpr const char* strings[] = {"Splitter", "Tabs"};
  return strings[static_cast<size_t>(type)];
}

Wt::WTabWidget* GetFirstTabWidgetHelper(Wt::WLayout& layout) {
  if (layout.count() == 0)
    return nullptr;
  auto& item = *layout.itemAt(0);
  if (item.layout())
    return GetFirstTabWidgetHelper(*item.layout());
  return dynamic_cast<Wt::WTabWidget*>(item.widget());
}

Wt::WTabWidget* GetFirstTabWidget(Wt::WBorderLayout& root_layout) {
  auto* item = root_layout.itemAt(Wt::LayoutPosition::Center);
  if (!item)
    return nullptr;
  if (item->layout())
    return GetFirstTabWidgetHelper(*item->layout());
  return dynamic_cast<Wt::WTabWidget*>(item->widget());
}

std::unique_ptr<Wt::WWidget> RemoveTabWorkaround(
    Wt::WTabWidget& tab_widget,
    Wt::WWidget& widget) {
  tab_widget.removeTab(&widget);
  return nullptr;
}

}  // namespace

class ViewManagerWtComponent::Block {
 public:
  explicit Block(std::unique_ptr<Wt::WWidget> widget)
      : widget_{std::move(widget)} {}
  explicit Block(std::unique_ptr<Wt::WLayout> layout)
      : layout_{std::move(layout)} {}

  void AddToLayout(Wt::WBoxLayout& layout) {
    if (widget_)
      layout.addWidget(std::move(widget_), 1);
    else if (layout_)
      layout.addLayout(std::move(layout_));
  }

  std::unique_ptr<Wt::WWidget> widget_;
  std::unique_ptr<Wt::WLayout> layout_;
};

ViewManagerWtComponent::LayoutNode::LayoutNode() = default;
ViewManagerWtComponent::LayoutNode::LayoutNode(LayoutNode&&) noexcept = default;
ViewManagerWtComponent::LayoutNode& ViewManagerWtComponent::LayoutNode::
operator=(LayoutNode&&) noexcept = default;
ViewManagerWtComponent::LayoutNode::~LayoutNode() = default;

ViewManagerWtComponent::ViewManagerWtComponent() = default;
ViewManagerWtComponent::~ViewManagerWtComponent() = default;

Wt::WLayout& ViewManagerWtComponent::root_layout() {
  return *root_pane_.root_layout_;
}

void ViewManagerWtComponent::SetCloseViewHandler(
    std::function<void(ViewId)> handler) {
  close_view_handler_ = std::move(handler);
}

void ViewManagerWtComponent::OpenLayout(std::span<const ViewInfo> views,
                                        const SavedLayout& layout) {
  RegisterViews(views);
  added_views_.clear();
  active_view_id_.reset();

  if (auto block = OpenLayoutBlock(layout.main))
    root_pane_.SetRootBlock(std::move(block));

  for (const auto& view : views_) {
    if (!IsViewAdded(view.id))
      AddView(view, std::nullopt);
  }
}

ViewManagerWtComponent::SavedLayout ViewManagerWtComponent::SaveLayout(
    std::span<const ViewInfo> views) {
  RegisterViews(views);

  SavedLayout layout;
  auto* item = root_pane_.root_layout_->itemAt(Wt::LayoutPosition::Center);
  if (!item)
    return layout;

  if (auto* child_layout = item->layout())
    SaveLayoutBlock(layout.main, *child_layout);
  else if (auto* tabs = dynamic_cast<Wt::WTabWidget*>(item->widget()))
    SaveLayoutBlock(layout.main, *tabs);
  return layout;
}

void ViewManagerWtComponent::AddView(const ViewInfo& view,
                                     std::optional<ViewId> active_view_id) {
  if (auto i = std::ranges::find(views_, view.id, &ViewInfo::id);
      i != views_.end()) {
    *i = view;
  } else {
    views_.emplace_back(view);
  }

  active_view_id_ = active_view_id;
  root_pane_.AddView(view);
}

bool ViewManagerWtComponent::RemoveView(ViewId view_id) {
  const auto* view = FindViewInfo(view_id);
  if (!view || !view->widget)
    return false;

  if (!root_pane_.RemoveView(*view))
    return false;

  std::erase(added_views_, view_id);
  std::erase_if(views_,
                [view_id](const ViewInfo& view) { return view.id == view_id; });
  if (active_view_id_ == view_id)
    active_view_id_.reset();
  return true;
}

void ViewManagerWtComponent::ActivateView(ViewId view_id) {
  const auto* view = FindViewInfo(view_id);
  if (!view || !view->widget)
    return;

  if (auto* tabs = root_pane_.GetTabWidget(view_id)) {
    auto index = tabs->indexOf(view->widget);
    if (index != -1)
      tabs->setCurrentIndex(index);
  }
  active_view_id_ = view_id;
}

void ViewManagerWtComponent::SplitView(ViewId view_id, bool vertically) {
  // Wt split operations were not implemented in the original ViewManagerWt.
}

void ViewManagerWtComponent::SetViewTitle(ViewId view_id,
                                          std::u16string_view title) {
  auto view = std::ranges::find(views_, view_id, &ViewInfo::id);
  if (view == views_.end())
    return;

  view->title = std::u16string{title};

  if (auto* tabs = root_pane_.GetTabWidget(view_id)) {
    auto index = tabs->indexOf(view->widget);
    if (index != -1)
      tabs->itemAt(index)->setText(Wt::WString{view->title});
  }
}

std::optional<ViewManagerWtComponent::ViewId>
ViewManagerWtComponent::GetActiveViewId() const {
  return active_view_id_;
}

void ViewManagerWtComponent::RootPane::SetRootBlock(std::unique_ptr<Block> block) {
  if (!block)
    return;

  if (block->layout_)
    root_layout_->add(std::move(block->layout_), Wt::LayoutPosition::Center);
  else
    root_layout_->addWidget(std::move(block->widget_),
                            Wt::LayoutPosition::Center);
}

bool ViewManagerWtComponent::RootPane::RemoveView(const ViewInfo& view) {
  assert(view.widget);
  if (!view.widget)
    return false;

  auto* pane = FindWidgetPane(*view.widget);
  assert(pane);
  if (!pane)
    return false;

  return pane->RemoveView(view);
}

bool ViewManagerWtComponent::DockPane::RemoveView(const ViewInfo& view) {
  assert(false);
  return false;
}

void ViewManagerWtComponent::DockPane::ClosePane() {}

ViewManagerWtComponent::DockSubPane::~DockSubPane() = default;

bool ViewManagerWtComponent::DockSubPane::RemoveView(const ViewInfo& view) {
  assert(view.widget);

  tab_widget_->removeTab(view.widget);

  assert(root_pane_.widget_data_.contains(view.widget));
  root_pane_.widget_data_.erase(view.widget);

  if (tab_widget_->count() == 0)
    ClosePane();

  return true;
}

void ViewManagerWtComponent::DockSubPane::ClosePane() {
  auto* item = root_pane_.root_layout_->findWidgetItem(tab_widget_);
  if (!item)
    return;

  if (item->parentLayout() == root_pane_.root_layout_) {
    assert(root_pane_.root_layout_->indexOf(item) != -1);
    root_pane_.root_layout_->removeItem(item);
  } else {
    Wt::WLayoutItem* other_item = nullptr;
    for (auto i = 0; i < item->parentLayout()->count(); ++i) {
      if (item->parentLayout()->itemAt(i) != item) {
        other_item = item->parentLayout()->itemAt(i);
        break;
      }
    }
    assert(other_item);
    auto* parent_layout = static_cast<Wt::WBoxLayout*>(other_item->parentLayout());
    auto other_item_ptr = parent_layout->removeItem(other_item);
    auto* super_parent_layout = parent_layout->parentLayout();
    if (super_parent_layout == root_pane_.root_layout_) {
      auto position = root_pane_.root_layout_->position(parent_layout);
      root_pane_.root_layout_->removeItem(parent_layout);
      root_pane_.root_layout_->add(std::move(other_item_ptr), position);
    } else {
      super_parent_layout->removeItem(parent_layout);
      super_parent_layout->addItem(std::move(other_item_ptr));
    }
  }
}

bool ViewManagerWtComponent::CenterPane::RemoveView(const ViewInfo& view) {
  assert(view.widget);

  auto* tab_widget = root_pane_.GetTabWidget(view.id);
  assert(tab_widget);
  if (!tab_widget)
    return false;

  RemoveTabWorkaround(*tab_widget, *view.widget);

  if (tab_widget !=
          root_pane_.root_layout_->widgetAt(Wt::LayoutPosition::Center) &&
      tab_widget->count() == 0) {
    tab_widget->removeFromParent();
  }

  assert(root_pane_.widget_data_.contains(view.widget));
  root_pane_.widget_data_.erase(view.widget);
  return true;
}

void ViewManagerWtComponent::RootPane::AddView(const ViewInfo& view) {
  if (view.dock) {
    auto side = view.dock_bottom ? DockSide::Bottom : DockSide::Left;
    GetOrCreateDockPane(side).AddView(view);
  } else {
    center_pane_.AddView(view);
  }
}

ViewManagerWtComponent::DockPane&
ViewManagerWtComponent::RootPane::GetOrCreateDockPane(DockSide side) {
  auto& pane = dock_pane(side);
  if (!pane)
    pane = std::make_unique<DockPane>(component_, *this, side);
  return *pane;
}

ViewManagerWtComponent::Pane* ViewManagerWtComponent::RootPane::FindWidgetPane(
    Wt::WWidget& widget) {
  auto i = widget_data_.find(&widget);
  return i != widget_data_.end() ? i->second.pane : nullptr;
}

ViewManagerWtComponent::DockPane::DockPane(ViewManagerWtComponent& component,
                                           RootPane& root_pane,
                                           DockSide side)
    : component_{component}, root_pane_{root_pane}, side_{side} {
  auto area = side_ == DockSide::Bottom ? Wt::LayoutPosition::South
                                        : Wt::LayoutPosition::West;
  auto* root_widget = root_pane_.root_layout_->addWidget(
      std::make_unique<Wt::WContainerWidget>(), area);
  if (area == Wt::LayoutPosition::South)
    root_widget->setHeight(200);
  else
    root_widget->setWidth(200);
  layout_ = root_widget->setLayout(std::make_unique<Wt::WVBoxLayout>());
  layout_->setContentsMargins(0, 0, 0, 0);
}

void ViewManagerWtComponent::DockPane::AddView(const ViewInfo& view) {
  assert(view.widget);
  assert(!component_.IsViewAdded(view.id));

  DockSubPane* subpane = nullptr;
  if (!subpanes_.empty() && view.tabify_existing_dock) {
    subpane = subpanes_.front().get();
  } else {
    subpane = subpanes_
                  .emplace_back(std::make_unique<DockSubPane>(
                      component_, root_pane_, *this))
                  .get();
  }

  subpane->AddView(view);
}

ViewManagerWtComponent::DockSubPane::DockSubPane(
    ViewManagerWtComponent& component,
    RootPane& root_pane,
    DockPane& dock_pane)
    : component_{component}, root_pane_{root_pane}, dock_pane_{dock_pane} {
  tab_widget_ =
      dock_pane.layout_->addWidget(root_pane_.CreateTabWidget(), /*stretch=*/1);

  if (dock_pane.side_ == DockSide::Bottom)
    tab_widget_->setHeight(200);
  else
    tab_widget_->setWidth(200);
}

void ViewManagerWtComponent::DockSubPane::AddView(const ViewInfo& view) {
  assert(view.widget);
  assert(!component_.IsViewAdded(view.id));

  auto* tab = tab_widget_->addTab(std::unique_ptr<Wt::WWidget>(view.widget),
                                  Wt::WString{view.title},
                                  Wt::ContentLoading::Eager);
  tab->setCloseable(true);

  view.widget->setHeight(Wt::WLength{99, Wt::LengthUnit::Percentage});

  assert(!root_pane_.widget_data_.contains(view.widget));
  root_pane_.widget_data_.try_emplace(view.widget, view.id, tab_widget_, this);

  component_.added_views_.emplace_back(view.id);
}

std::unique_ptr<ViewManagerWtComponent::Block>
ViewManagerWtComponent::OpenLayoutBlock(const LayoutNode& block) {
  if (block.type == LayoutNode::Type::Tabs) {
    std::vector<const ViewInfo*> tab_views;
    for (ViewId view_id : block.tabs) {
      auto* view = FindViewInfo(view_id);
      if (!view)
        continue;

      if (view->dock)
        root_pane_.AddView(*view);
      else
        tab_views.emplace_back(view);
    }

    if (tab_views.empty())
      return nullptr;

    auto tab_widget = root_pane_.CreateTabWidget();
    for (const auto* view : tab_views) {
      if (!IsViewAdded(view->id)) {
        auto* tab =
            tab_widget->addTab(std::unique_ptr<Wt::WWidget>(view->widget),
                               Wt::WString{view->title},
                               Wt::ContentLoading::Eager);
        tab->setCloseable(true);
        root_pane_.RegisterCenterView(*view, *tab_widget);
        added_views_.emplace_back(view->id);
      }
    }
    return std::make_unique<Block>(std::move(tab_widget));
  }

  auto child1 = block.left ? OpenLayoutBlock(*block.left) : nullptr;
  auto child2 = block.right ? OpenLayoutBlock(*block.right) : nullptr;
  if (!child1)
    return child2;
  if (!child2)
    return child1;

  auto layout = block.split_vertical
                    ? static_cast<std::unique_ptr<Wt::WBoxLayout>>(
                          std::make_unique<Wt::WVBoxLayout>())
                    : static_cast<std::unique_ptr<Wt::WBoxLayout>>(
                          std::make_unique<Wt::WHBoxLayout>());
  child1->AddToLayout(*layout);
  child2->AddToLayout(*layout);
  return std::make_unique<Block>(std::move(layout));
}

void ViewManagerWtComponent::CenterPane::AddView(const ViewInfo& view) {
  assert(view.widget);
  assert(!component_.IsViewAdded(view.id));

  auto* tab_widget = component_.active_view_id_
                         ? root_pane_.GetTabWidget(*component_.active_view_id_)
                         : nullptr;
  if (!tab_widget)
    tab_widget = GetFirstTabWidget(*root_pane_.root_layout_);

  if (!tab_widget) {
    auto root_widget = root_pane_.CreateTabWidget();
    tab_widget = root_widget.get();
    root_pane_.SetRootBlock(std::make_unique<Block>(std::move(root_widget)));
  }

  auto* tab = tab_widget->addTab(std::unique_ptr<Wt::WWidget>(view.widget),
                                 Wt::WString{view.title},
                                 Wt::ContentLoading::Eager);
  tab->setCloseable(true);

  assert(!root_pane_.widget_data_.contains(view.widget));
  root_pane_.widget_data_.try_emplace(view.widget, view.id, tab_widget, this);

  component_.added_views_.emplace_back(view.id);
}

std::unique_ptr<Wt::WTabWidget>
ViewManagerWtComponent::RootPane::CreateTabWidget() {
  auto tab_widget = std::make_unique<Wt::WTabWidget>();
  tab_widget->setAttributeValue(sc_layoutWidgetTypeProp,
                                ToString(LayoutWidgetType::Tabs));

  tab_widget->tabClosed().connect([this, &tab_widget = *tab_widget](int index) {
    auto* view = component_.FindViewInfoByWidget(tab_widget.widget(index));
    if (view && component_.close_view_handler_)
      component_.close_view_handler_(view->id);
  });

  return tab_widget;
}

void ViewManagerWtComponent::RootPane::RegisterCenterView(
    const ViewInfo& view,
    Wt::WTabWidget& tab_widget) {
  assert(view.widget);
  assert(!widget_data_.contains(view.widget));
  widget_data_.try_emplace(view.widget, view.id, &tab_widget, &center_pane_);
}

Wt::WTabWidget* ViewManagerWtComponent::RootPane::GetTabWidget(ViewId view_id) {
  const auto* view = component_.FindViewInfo(view_id);
  if (!view || !view->widget)
    return nullptr;

  auto i = widget_data_.find(view->widget);
  return i != widget_data_.end() ? i->second.tab_widget : nullptr;
}

ViewManagerWtComponent::RootPane::RootPane(ViewManagerWtComponent& component)
    : component_{component} {
  root_layout_ = new Wt::WBorderLayout();
  root_layout_->setContentsMargins(0, 0, 0, 0);
  root_layout_->addWidget(CreateTabWidget(), Wt::LayoutPosition::Center);
}

void ViewManagerWtComponent::SaveLayoutBlock(LayoutNode& block,
                                             Wt::WLayout& layout) const {
  block.type = LayoutNode::Type::Split;
  block.split_vertical = dynamic_cast<Wt::WVBoxLayout*>(&layout) != nullptr;
  block.left = std::make_unique<LayoutNode>();
  block.right = std::make_unique<LayoutNode>();

  if (auto* item = layout.itemAt(0)) {
    if (auto* child_layout = item->layout())
      SaveLayoutBlock(*block.left, *child_layout);
    else if (auto* tabs = dynamic_cast<Wt::WTabWidget*>(item->widget()))
      SaveLayoutBlock(*block.left, *tabs);
  }

  if (auto* item = layout.itemAt(1)) {
    if (auto* child_layout = item->layout())
      SaveLayoutBlock(*block.right, *child_layout);
    else if (auto* tabs = dynamic_cast<Wt::WTabWidget*>(item->widget()))
      SaveLayoutBlock(*block.right, *tabs);
  }
}

void ViewManagerWtComponent::SaveLayoutBlock(LayoutNode& block,
                                             Wt::WTabWidget& tab_widget) const {
  block.type = LayoutNode::Type::Tabs;
  block.tabs.clear();
  for (int i = 0; i < tab_widget.count(); ++i) {
    auto* view = FindViewInfoByWidget(tab_widget.widget(i));
    if (view)
      block.tabs.emplace_back(view->id);
  }
}

void ViewManagerWtComponent::RegisterViews(std::span<const ViewInfo> views) {
  views_.assign(views.begin(), views.end());
}

const ViewManagerWtComponent::ViewInfo* ViewManagerWtComponent::FindViewInfo(
    ViewId view_id) const {
  auto i = std::ranges::find(views_, view_id, &ViewInfo::id);
  return i != views_.end() ? &*i : nullptr;
}

const ViewManagerWtComponent::ViewInfo*
ViewManagerWtComponent::FindViewInfoByWidget(const Wt::WWidget* widget) const {
  if (!widget)
    return nullptr;

  auto i = std::ranges::find_if(views_, [widget](const ViewInfo& view) {
    return view.widget == widget;
  });
  return i != views_.end() ? &*i : nullptr;
}

bool ViewManagerWtComponent::IsViewAdded(ViewId view_id) const {
  return std::ranges::find(added_views_, view_id) != added_views_.end();
}
