#pragma once

#include "components/main/view_manager.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/controls/multi_split_view.h"
#include "ui/views/focus/focus_manager.h"

namespace views {
class MultiSplitView;
class View;
class Widget;
}  // namespace views

class ViewManagerViews final : public ViewManager,
                               private views::FocusChangeListener,
                               private views::MultiSplitView::Controller {
 public:
  ViewManagerViews(views::View& parent_view, ViewManagerDelegate& delegate);
  ~ViewManagerViews();

  // Called after native widget is installed.
  void Init();

  views::View& GetView();

  // ViewManager
  virtual void SetViewTitle(OpenedView& view, const base::string16& title) override;
  virtual void ActivateView(OpenedView& view) override;
  virtual void CloseView(OpenedView& view) override;

 protected:
  // ViewManager
  virtual void OpenLayout(Page& page, const PageLayout& layout) override;
  virtual void SaveLayout(PageLayout& layout) override;
  virtual void AddView(OpenedView& view) override;

 private:
  void OpenLayoutBlock(const PageLayoutBlock& block,
                       views::MultiSplitPane& pane);
  void SaveLayoutBlock(PageLayoutBlock& block, views::MultiSplitPane& pane);

  OpenedView* FindViewByViewsView(views::View* view);
  OpenedView* FindFirstDataView(views::MultiSplitPane& from_pane);

  // views::FocusChangeListener
  virtual void OnFocusChanged(views::View* focused_before,
                              views::View* focused_now) override;

  // MultiSplitView::Controller
  virtual void OnViewClosed(views::View& view) override;
  virtual void OnShowViewTabContextMenu(views::View& view,
                                        gfx::Point point) override;

  views::FocusManager* focus_manager_ = nullptr;
  std::unique_ptr<views::MultiSplitView> dock_container_;
};
