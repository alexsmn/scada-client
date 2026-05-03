#pragma once

#include "main_window/qt/view_manager_qt_component.h"
#include "main_window/view_manager.h"

#include <vector>

class QMainWindow;

class ViewManagerQt final : public QObject, public ViewManager {
  Q_OBJECT

 public:
  ViewManagerQt(QMainWindow& main_window, ViewManagerDelegate& delegate);
  ~ViewManagerQt();

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
  using ComponentLayoutNode = ViewManagerQtComponent::LayoutNode;
  using ComponentSavedLayout = ViewManagerQtComponent::SavedLayout;
  using ComponentViewInfo = ViewManagerQtComponent::ViewInfo;

  ComponentViewInfo GetComponentViewInfo(OpenedView& view) const;
  std::vector<ComponentViewInfo> GetComponentViewInfos() const;
  ViewManagerQtComponent::ViewId GetComponentViewId(
      const OpenedView& view) const;
  OpenedView* FindViewByComponentId(
      ViewManagerQtComponent::ViewId view_id) const;

  ComponentSavedLayout ToComponentLayout(const PageLayout& layout) const;
  ComponentLayoutNode ToComponentLayoutNode(
      const PageLayoutBlock& block) const;
  void FromComponentLayout(const ComponentSavedLayout& component_layout,
                           PageLayout& layout) const;
  void FromComponentLayoutNode(const ComponentLayoutNode& component_block,
                               PageLayoutBlock& block) const;

  ViewManagerQtComponent component_;
};
