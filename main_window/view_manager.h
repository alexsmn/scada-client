#pragma once

#include <list>
#include <memory>
#include <string>

class OpenedView;
class Page;
class PageLayout;
class PageLayoutBlock;
class ViewManagerDelegate;
class WindowDefinition;
struct WindowInfo;

class ViewManager {
 public:
  virtual ~ViewManager();

  ViewManager(const ViewManager&) = delete;
  ViewManager& operator=(const ViewManager&) = delete;

  // WARNING: List is required for deletion during iterator.
  using Views = std::list<OpenedView*>;
  const Views& views() const { return views_; }

  OpenedView* FindViewByType(std::string_view window_type) const;

  OpenedView* CreateView(WindowDefinition& def,
                         const OpenedView* after_view = nullptr);

  OpenedView* OpenView(const WindowDefinition& def,
                       bool activate,
                       const OpenedView* after_view);

  Page& current_page() const { return *current_page_; }
  bool is_closing_page() const { return closing_page_; }
  void OpenPage(const Page& page);
  void SavePage();
  void ClosePage();

  virtual OpenedView* GetActiveView() = 0;
  virtual void ActivateView(const OpenedView& view) = 0;
  virtual void CloseView(OpenedView& view) = 0;

  virtual void SetViewTitle(OpenedView& view, const std::u16string& title) = 0;

  virtual void SplitView(OpenedView& view, bool vertically) = 0;

 protected:
  explicit ViewManager(ViewManagerDelegate& delegate);

  void SetActiveView(OpenedView* view);
  void DestroyView(OpenedView& view);

  OpenedView* FindViewByID(int id) const;

  bool IsViewAdded(OpenedView& opened_view) const;

  virtual void OpenLayout(Page& page, const PageLayout& layout) = 0;
  virtual void SaveLayout(PageLayout& layout) = 0;

  virtual void AddView(OpenedView& view) = 0;

  ViewManagerDelegate& delegate_;

  // TODO: Use unique_ptr instead of raw pointer.
  Views views_;
  Views added_views_;

  OpenedView* active_view_ = nullptr;

  bool opening_layout_ = false;
  bool closing_page_ = false;

  std::unique_ptr<Page> current_page_;
};
