#pragma once

#include "components/main/view_manager.h"

namespace Wt {
class WBorderLayout;
class WBoxLayout;
class WContainerWidget;
class WLayout;
class WTabWidget;
class WWidget;
}  // namespace Wt

class ViewManager2Wt final : public ViewManager {
 public:
  explicit ViewManager2Wt(ViewManagerDelegate& delegate);
  ~ViewManager2Wt();

  Wt::WLayout& root_layout();

  // ViewManager
  virtual OpenedView* GetActiveView() override;
  virtual void SetViewTitle(OpenedView& view,
                            const std::u16string& title) override;
  virtual void ActivateView(OpenedView& view) override;
  virtual void CloseView(OpenedView& view) override;
  virtual void SplitView(OpenedView& view, bool vertically) override;

 protected:
  // ViewManager
  virtual void OpenLayout(Page& page, const PageLayout& layout) override;
  virtual void SaveLayout(PageLayout& layout) override;
  virtual void AddView(OpenedView& view) override;

 private:
  OpenedView* FindViewByWidget(const Wt::WWidget* widget);

  void OpenLayoutBlock(const Page& page, const PageLayoutBlock& block);

  std::unique_ptr<Wt::WTabWidget> CreateTabWidget();

  Wt::WBorderLayout* root_layout_ = nullptr;

  enum class DockSide { Left, Bottom, Count };

  Wt::WTabWidget* center_tab_widget_ = nullptr;
  Wt::WTabWidget* dock_tab_widgets_[static_cast<size_t>(DockSide::Count)];
};
