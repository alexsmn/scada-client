#pragma once

#include "main_window/view_manager.h"
#include "aui/wt/view_manager_wt_component.h"

#include <vector>

namespace Wt {
class WLayout;
}  // namespace Wt

class ViewManagerWt final : public ViewManager {
 public:
  explicit ViewManagerWt(ViewManagerDelegate& delegate);
  ~ViewManagerWt();

  Wt::WLayout& root_layout();

  // ViewManager
  virtual OpenedView* GetActiveView() override;
  virtual void SetViewTitle(OpenedView& view,
                            const std::u16string& title) override;
  virtual void ActivateView(const OpenedView& view) override;
  virtual void CloseView(OpenedView& view) override;
  virtual void SplitView(OpenedView& view, bool vertically) override;

 protected:
  // ViewManager
  virtual void OpenLayout(Page& page, const PageLayout& layout) override;
  virtual void SaveLayout(PageLayout& layout) override;
  virtual void AddView(OpenedView& view) override;

 private:
  using ComponentLayoutNode = ViewManagerWtComponent::LayoutNode;
  using ComponentSavedLayout = ViewManagerWtComponent::SavedLayout;
  using ComponentViewInfo = ViewManagerWtComponent::ViewInfo;

  ComponentViewInfo GetComponentViewInfo(OpenedView& view) const;
  std::vector<ComponentViewInfo> GetComponentViewInfos() const;
  ViewManagerWtComponent::ViewId GetComponentViewId(
      const OpenedView& view) const;
  OpenedView* FindViewByComponentId(
      ViewManagerWtComponent::ViewId view_id) const;

  ComponentSavedLayout ToComponentLayout(const PageLayout& layout) const;
  ComponentLayoutNode ToComponentLayoutNode(
      const PageLayoutBlock& block) const;
  void FromComponentLayout(const ComponentSavedLayout& component_layout,
                           PageLayout& layout) const;
  void FromComponentLayoutNode(const ComponentLayoutNode& component_block,
                               PageLayoutBlock& block) const;

  ViewManagerWtComponent component_;
};
