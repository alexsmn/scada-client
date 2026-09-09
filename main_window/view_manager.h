#pragma once

#include "aui/view_manager.h"
#include "base/lifetime.h"
#include "main_window/opened_view/opened_view.h"
#include "profile/page_layout.h"

#include <cstdint>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class OpenedView;
class Page;
class PageLayout;
class PageLayoutBlock;
class QMainWindow;
class ViewManagerDelegate;
class WindowDefinition;
struct WindowInfo;

class ViewManager {
 public:
#if defined(UI_QT)
  ViewManager(QMainWindow& main_window, ViewManagerDelegate& delegate);
#endif

  virtual ~ViewManager();

  ViewManager(const ViewManager&) = delete;
  ViewManager& operator=(const ViewManager&) = delete;

  // WARNING: List is required for deletion during iterator.
  using Views = std::list<OpenedView*>;
  const Views& views() const SCADA_LIFETIME_BOUND { return views_; }

  OpenedView* FindViewByType(std::string_view window_type) const;

  // The view of `window_type` already showing `item_path`, or null. Used to
  // deduplicate WIN_SINGLE_ITEM windows, whose identity is the pair rather
  // than the type alone.
  OpenedView* FindViewByTypeAndItem(std::string_view window_type,
                                    std::string_view item_path) const;

  // The node path a single-item window is bound to; empty when the definition
  // carries no item.
  static std::string_view GetSingleItemPath(const WindowDefinition& def);

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

  OpenedView* GetActiveView();
  void ActivateView(const OpenedView& view);
  void CloseView(OpenedView& view);

  void SetViewTitle(OpenedView& view, const std::u16string& title);

  void SplitView(OpenedView& view, bool vertically);

 protected:
  void SetActiveView(OpenedView* view);
  void DestroyView(OpenedView& view);

  OpenedView* FindViewByID(int id) const;

  scada::aui::ViewManagerViewId GetComponentViewId(
      const OpenedView& view) const;
  scada::aui::ViewManagerViewInfo GetComponentViewInfo(OpenedView& view) const;
  OpenedView* FindViewByComponentId(
      scada::aui::ViewManagerViewId view_id) const;
  std::vector<scada::aui::ViewManagerViewInfo> GetComponentViewInfos() const;
  scada::aui::ViewManagerSavedLayout ToComponentLayout(
      const PageLayout& layout) const;
  scada::aui::ViewManagerLayoutNode ToComponentLayoutNode(
      const PageLayoutBlock& block) const;
  void FromComponentLayout(
      const scada::aui::ViewManagerSavedLayout& component_layout,
      PageLayout& layout) const;
  void FromComponentLayoutNode(
      const scada::aui::ViewManagerLayoutNode& component_block,
      PageLayoutBlock& block) const;

  void OpenLayout(Page& page, const PageLayout& layout);
  void SaveLayout(PageLayout& layout);

  void AddView(OpenedView& view);

  ViewManagerDelegate& delegate_;

  // TODO: Use unique_ptr instead of raw pointer.
  Views views_;

  OpenedView* active_view_ = nullptr;

  bool opening_layout_ = false;
  bool closing_page_ = false;

  std::unique_ptr<Page> current_page_;

  scada::aui::ViewManagerComponent component_;
};
