#pragma once

#include <list>
#include <memory>

#include "base/strings/string16.h"

class OpenedView;
class Page;
class PageLayout;
class PageLayoutBlock;
class ViewManagerDelegate;
class WindowDefinition;

class ViewManager {
 public:
  virtual ~ViewManager();

  ViewManager(const ViewManager&) = delete;
  ViewManager& operator=(const ViewManager&) = delete;

  // WARNING: List allows deletion without iterator corruption.
  struct ViewInfo {
    OpenedView* view;
    WindowDefinition* definition;
  };
  typedef std::list<ViewInfo> Views;

  const Views& views() const { return views_; }
  OpenedView* FindViewByID(int id) const;
  OpenedView* FindViewByType(unsigned type) const;

  OpenedView* CreateView(WindowDefinition& def, OpenedView* after_view);
  OpenedView* OpenView(const WindowDefinition& def,
                       bool make_active,
                       OpenedView* after_view);

  OpenedView* active_view() const { return active_view_; }

  Page& current_page() const { return *current_page_; }
  bool is_closing_page() const { return closing_page_; }
  void OpenPage(const Page& page);
  void SavePage();
  void ClosePage();

  virtual void ActivateView(OpenedView& view) = 0;
  virtual void CloseView(OpenedView& view) = 0;

  virtual void SetViewTitle(OpenedView& view, const base::string16& title) = 0;

 protected:
  explicit ViewManager(ViewManagerDelegate& delegate);

  void SetActiveView(OpenedView* view);
  void DestroyView(OpenedView& view);

  virtual void OpenLayout(Page& page, const PageLayout& layout) = 0;
  virtual void SaveLayout(PageLayout& layout) = 0;

  virtual void AddView(OpenedView& view) = 0;

  ViewManagerDelegate& delegate_;

  Views views_;

  OpenedView* active_view_ = nullptr;

  bool opening_layout_ = false;
  bool closing_page_ = false;

  std::unique_ptr<Page> current_page_;
};
