#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace Wt {
class WBorderLayout;
class WBoxLayout;
class WContainerWidget;
class WLayout;
class WTabWidget;
class WWidget;
}  // namespace Wt

class ViewManagerWtComponent final {
 public:
  using ViewId = std::uintptr_t;

  struct ViewInfo {
    ViewId id = 0;
    Wt::WWidget* widget = nullptr;
    std::u16string title;
    bool dock = false;
    bool dock_bottom = false;
    bool tabify_existing_dock = true;
  };

  struct LayoutNode {
    enum class Type { Tabs, Split };

    LayoutNode();
    LayoutNode(LayoutNode&&) noexcept;
    LayoutNode& operator=(LayoutNode&&) noexcept;
    ~LayoutNode();

    Type type = Type::Tabs;
    std::vector<ViewId> tabs;
    bool split_vertical = false;
    int split_pos = -1;
    std::unique_ptr<LayoutNode> left;
    std::unique_ptr<LayoutNode> right;
  };

  struct SavedLayout {
    LayoutNode main;
  };

  ViewManagerWtComponent();
  ~ViewManagerWtComponent();

  Wt::WLayout& root_layout();

  void SetCloseViewHandler(std::function<void(ViewId)> handler);

  void OpenLayout(std::span<const ViewInfo> views, const SavedLayout& layout);
  SavedLayout SaveLayout(std::span<const ViewInfo> views);

  void AddView(const ViewInfo& view, std::optional<ViewId> active_view_id);
  bool RemoveView(ViewId view_id);
  void ActivateView(ViewId view_id);
  void SplitView(ViewId view_id, bool vertically);
  void SetViewTitle(ViewId view_id, std::u16string_view title);

  std::optional<ViewId> GetActiveViewId() const;

 private:
  class Block;

  enum class DockSide { Left, Top, Right, Bottom, Count };

  class Pane {
   public:
    virtual ~Pane() = default;

    virtual void AddView(const ViewInfo& view) = 0;
    virtual bool RemoveView(const ViewInfo& view) = 0;
  };

  class RootPane;
  class DockPane;

  class DockSubPane : public Pane {
   public:
    DockSubPane(ViewManagerWtComponent& component,
                RootPane& root_pane,
                DockPane& dock_pane);

    ~DockSubPane();

    void ClosePane();

    void AddView(const ViewInfo& view) override;
    bool RemoveView(const ViewInfo& view) override;

   private:
    ViewManagerWtComponent& component_;
    RootPane& root_pane_;
    DockPane& dock_pane_;

    Wt::WTabWidget* tab_widget_ = nullptr;
  };

  class DockPane : public Pane {
   public:
    DockPane(ViewManagerWtComponent& component, RootPane& root_pane,
             DockSide side);

    void ClosePane();

    void AddView(const ViewInfo& view) override;
    bool RemoveView(const ViewInfo& view) override;

   private:
    ViewManagerWtComponent& component_;
    RootPane& root_pane_;
    const DockSide side_;

    Wt::WBoxLayout* layout_ = nullptr;

    std::vector<std::unique_ptr<DockSubPane>> subpanes_;

    friend class DockSubPane;
  };

  class CenterPane : public Pane {
   public:
    CenterPane(ViewManagerWtComponent& component, RootPane& root_pane)
        : component_{component}, root_pane_{root_pane} {}

    void AddView(const ViewInfo& view) override;
    bool RemoveView(const ViewInfo& view) override;

   private:
    ViewManagerWtComponent& component_;
    RootPane& root_pane_;
  };

  class RootPane : public Pane {
   public:
    explicit RootPane(ViewManagerWtComponent& component);
    RootPane(const RootPane&) = delete;
    RootPane& operator=(const RootPane&) = delete;

    DockPane& GetOrCreateDockPane(DockSide side);

    Wt::WTabWidget* GetTabWidget(ViewId view_id);
    Pane* FindWidgetPane(Wt::WWidget& widget);

    std::unique_ptr<Wt::WTabWidget> CreateTabWidget();
    void RegisterCenterView(const ViewInfo& view, Wt::WTabWidget& tab_widget);

    void SetRootBlock(std::unique_ptr<Block> block);

    void AddView(const ViewInfo& view) override;
    bool RemoveView(const ViewInfo& view) override;

   private:
    std::unique_ptr<DockPane>& dock_pane(DockSide side) {
      return dock_panes_[static_cast<size_t>(side)];
    }

    ViewManagerWtComponent& component_;

    CenterPane center_pane_{component_, *this};

    std::unique_ptr<DockPane> dock_panes_[static_cast<size_t>(DockSide::Count)];

   public:
    struct WidgetData {
      ViewId view_id = 0;
      Wt::WTabWidget* tab_widget = nullptr;
      Pane* pane = nullptr;
    };

    std::unordered_map<Wt::WWidget*, WidgetData> widget_data_;

    Wt::WBorderLayout* root_layout_ = nullptr;
  };

  std::unique_ptr<Block> OpenLayoutBlock(const LayoutNode& block);
  void SaveLayoutBlock(LayoutNode& block, Wt::WLayout& layout) const;
  void SaveLayoutBlock(LayoutNode& block, Wt::WTabWidget& tab_widget) const;

  void RegisterViews(std::span<const ViewInfo> views);
  const ViewInfo* FindViewInfo(ViewId view_id) const;
  const ViewInfo* FindViewInfoByWidget(const Wt::WWidget* widget) const;
  bool IsViewAdded(ViewId view_id) const;

  RootPane root_pane_{*this};
  std::vector<ViewInfo> views_;
  std::vector<ViewId> added_views_;
  std::optional<ViewId> active_view_id_;
  std::function<void(ViewId)> close_view_handler_;
};
