#pragma once

#include "components/main/view_manager.h"

#include <unordered_map>

namespace Wt {
class WBorderLayout;
class WBoxLayout;
class WContainerWidget;
class WLayout;
class WTabWidget;
class WWidget;
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
  virtual void ActivateView(OpenedView& view) override;
  virtual void CloseView(OpenedView& view) override;
  virtual void SplitView(OpenedView& view, bool vertically) override;

 protected:
  // ViewManager
  virtual void OpenLayout(Page& page, const PageLayout& layout) override;
  virtual void SaveLayout(PageLayout& layout) override;
  virtual void AddView(OpenedView& view) override;

 private:
  class Block;

  OpenedView* FindViewByWidget(const Wt::WWidget* widget);

  std::unique_ptr<Block> OpenLayoutBlock(const Page& page,
                                         const PageLayoutBlock& block);

  enum class DockSide { Left, Top, Right, Bottom, Count };

  class Pane {
   public:
    virtual ~Pane() = default;

    virtual void AddView(OpenedView& view) = 0;
    virtual std::unique_ptr<Wt::WWidget> RemoveView(OpenedView& view) = 0;
  };

  class RootPane;
  class DockPane;

  class DockSubPane : public Pane {
   public:
    DockSubPane(ViewManagerWt& view_manager,
                RootPane& root_pane,
                DockPane& dock_pane);

    void ClosePane();

    virtual void AddView(OpenedView& view) override;
    virtual std::unique_ptr<Wt::WWidget> RemoveView(OpenedView& view) override;

   private:
    ViewManagerWt& view_manager_;
    RootPane& root_pane_;
    DockPane& dock_pane_;

    Wt::WTabWidget* tab_widget_ = nullptr;
  };

  class DockPane : public Pane {
   public:
    DockPane(ViewManagerWt& view_manager, RootPane& root_pane, DockSide side);

    void ClosePane();

    virtual void AddView(OpenedView& view) override;
    virtual std::unique_ptr<Wt::WWidget> RemoveView(OpenedView& view) override;

   private:
    ViewManagerWt& view_manager_;
    RootPane& root_pane_;
    const DockSide side_;

    Wt::WBoxLayout* layout_ = nullptr;

    std::vector<DockSubPane> subpanes_;

    friend class DockSubPane;
  };

  class CenterPane : public Pane {
   public:
    CenterPane(ViewManagerWt& view_manager, RootPane& root_pane)
        : view_manager_{view_manager}, root_pane_{root_pane} {}

    virtual void AddView(OpenedView& view) override;
    virtual std::unique_ptr<Wt::WWidget> RemoveView(OpenedView& view) override;

   private:
    ViewManagerWt& view_manager_;
    RootPane& root_pane_;
  };

  class RootPane : public Pane {
   public:
    explicit RootPane(ViewManagerWt& view_manager);

    DockPane& GetOrCreateDockPane(DockSide side);

    Wt::WTabWidget* GetTabWidget(OpenedView& opened_view);
    Pane* FindWidgetPane(Wt::WWidget& widget);

    std::unique_ptr<Wt::WTabWidget> CreateTabWidget();

    void SetRootBlock(std::unique_ptr<Block> block);

    Wt::WLayout* GetParentLayout(Wt::WTabWidget& tab_widget);

    // Pane
    virtual void AddView(OpenedView& view) override;
    virtual std::unique_ptr<Wt::WWidget> RemoveView(OpenedView& view) override;

   private:
    std::unique_ptr<DockPane>& dock_pane(DockSide side) {
      return dock_panes_[static_cast<size_t>(side)];
    }

    ViewManagerWt& view_manager_;

    CenterPane center_pane_{view_manager_, *this};

    std::unique_ptr<DockPane> dock_panes_[static_cast<size_t>(DockSide::Count)];

   public:
    struct WidgetData {
      Wt::WTabWidget* tab_widget = nullptr;
      Pane* pane = nullptr;
    };

    std::unordered_map<Wt::WWidget*, WidgetData> widget_data_;

    struct TabWidgetData {
      Wt::WLayout* parent_layout_ = nullptr;
    };

    std::unordered_map<Wt::WTabWidget*, TabWidgetData> tab_widget_data_;

    Wt::WBorderLayout* root_layout_ = nullptr;
  };

  RootPane root_pane_{*this};
};
