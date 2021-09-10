#include "components/main/wt/view_manager_wt.h"

#include "base/auto_reset.h"
#include "common_resources.h"
#include "components/main/opened_view.h"
#include "components/main/view_manager_delegate.h"
#include "services/page.h"
#include "window_info.h"

#include <wt/WApplication.h>
#include <wt/WBorderLayout.h>
#include <wt/WContainerWidget.h>
#include <wt/WHBoxLayout.h>
#include <wt/WMenu.h>
#include <wt/WMenuItem.h>
#include <wt/WStackedWidget.h>
#include <wt/WTabWidget.h>
#include <wt/WVBoxLayout.h>
#include <wt/WWidgetItem.h>

namespace {

const char sc_layoutWidgetTypeProp[] = "LayoutWidgetType";

enum class LayoutWidgetType { Splitter, Tabs, Window, Panel, Count };

std::string ToString(LayoutWidgetType type) {
  constexpr const char* strings[] = {"Splitter", "Tabs", "Window", "Panel"};
  return strings[static_cast<size_t>(type)];
}

LayoutWidgetType ToLayoutWidgetType(std::string_view str) {
  for (size_t i = 0; i < static_cast<size_t>(LayoutWidgetType::Count); ++i) {
    auto type = static_cast<LayoutWidgetType>(i);
    if (ToString(type) == str)
      return type;
  }
  return LayoutWidgetType::Count;
}

Wt::WTabWidget* GetFirstTabWidgetHelper(Wt::WLayout& layout) {
  assert(layout.count() == 0);
  auto& item = *layout.itemAt(0);
  if (item.layout())
    return GetFirstTabWidgetHelper(*item.layout());
  else
    return static_cast<Wt::WTabWidget*>(item.widget());
}

Wt::WTabWidget* GetFirstTabWidget(Wt::WBorderLayout& root_layout) {
  auto* item = root_layout.itemAt(Wt::LayoutPosition::Center);
  if (!item)
    return nullptr;
  if (item->layout())
    return GetFirstTabWidgetHelper(*item->layout());
  else
    return static_cast<Wt::WTabWidget*>(item->widget());
}

}  // namespace

class ViewManagerWt::Block {
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

// ViewManagerWt

ViewManagerWt::ViewManagerWt(ViewManagerDelegate& delegate)
    : ViewManager{delegate} {}

ViewManagerWt::~ViewManagerWt() {}

Wt::WLayout& ViewManagerWt::root_layout() {
  return *root_pane_.root_layout_;
}

void ViewManagerWt::RootPane::SetRootBlock(std::unique_ptr<Block> block) {
  if (block->layout_)
    root_layout_->add(std::move(block->layout_), Wt::LayoutPosition::Center);
  else
    root_layout_->addWidget(std::move(block->widget_),
                            Wt::LayoutPosition::Center);
}

void ViewManagerWt::OpenLayout(Page& page, const PageLayout& layout) {
  Wt::WApplication::UpdateLock update_lock{Wt::WApplication::instance()};

  if (auto block = OpenLayoutBlock(page, layout.main))
    root_pane_.SetRootBlock(std::move(block));

  // Open windows not opended by layout.
  for (auto* opened_view : views_) {
    if (!IsViewAdded(*opened_view))
      AddView(*opened_view);
  }
}

OpenedView* ViewManagerWt::FindViewByWidget(const Wt::WWidget* widget) {
  auto i = std::find_if(views_.begin(), views_.end(),
                        [widget](OpenedView* opened_view) {
                          return opened_view->view() == widget;
                        });
  return i != views_.end() ? *i : nullptr;
}

void ViewManagerWt::ActivateView(OpenedView& opened_view) {}

void ViewManagerWt::CloseView(OpenedView& opened_view) {
  root_pane_.RemoveView(opened_view);
}

std::unique_ptr<Wt::WWidget> ViewManagerWt::RootPane::RemoveView(
    OpenedView& opened_view) {
  // TODO: Describe the scenario in a comment.
  assert(opened_view.view());
  if (!opened_view.view())
    return nullptr;

  auto* pane = FindWidgetPane(*opened_view.view());
  if (!pane)
    return nullptr;

  auto w = pane->RemoveView(opened_view);

  opened_view.ReleaseView();
  view_manager_.DestroyView(opened_view);

  return w;
}

std::unique_ptr<Wt::WWidget> ViewManagerWt::DockPane::RemoveView(
    OpenedView& opened_view) {
  assert(false);
  return nullptr;
}

void ViewManagerWt::DockPane::ClosePane() {}

std::unique_ptr<Wt::WWidget> ViewManagerWt::DockSubPane::RemoveView(
    OpenedView& opened_view) {
  assert(opened_view.view());

  auto w = tab_widget_->removeTab(opened_view.view());

  assert(root_pane_.widget_data_.find(w.get()) !=
         root_pane_.widget_data_.end());
  root_pane_.widget_data_.erase(w.get());

  if (tab_widget_->count() == 0)
    ClosePane();

  return w;
}

void ViewManagerWt::DockSubPane::ClosePane() {
  auto* item = root_pane_.root_layout_->findWidgetItem(tab_widget_);
  assert(item);

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
    auto* parent_layout =
        static_cast<Wt::WBoxLayout*>(other_item->parentLayout());
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

// A workaround for a |WMenuItem::removeContents| bug that makes
// |QTabWidget::removeTab| to return null.
// https://github.com/emweb/wt/blob/c7802997dfa4a37db86b4a923d9ff61042907531/src/Wt/WMenuItem.C#L492
std::unique_ptr<Wt::WWidget> RemoveTabWorkaround(Wt::WTabWidget& tab_widget,
                                                 Wt::WWidget& widget) {
  /*int index = tab_widget.indexOf(&widget);
  assert(index != -1);
  auto* item = tab_widget.itemAt(index);
  auto w = item->removeWidget(&widget);
  assert(w.get() == &widget);
  tab_widget.removeChild(item);
  return w;*/
  //  std::unique_ptr<Wt::WWidget> w{&widget};
  tab_widget.removeTab(&widget);
  return nullptr;
}

std::unique_ptr<Wt::WWidget> ViewManagerWt::CenterPane::RemoveView(
    OpenedView& opened_view) {
  assert(opened_view.view());

  auto* tab_widget = root_pane_.GetTabWidget(opened_view);
  assert(tab_widget);

  auto w = RemoveTabWorkaround(*tab_widget, *opened_view.view());

  if (tab_widget !=
          root_pane_.root_layout_->widgetAt(Wt::LayoutPosition::Center) &&
      tab_widget->count() == 0) {
    tab_widget->removeFromParent();
  }

  assert(root_pane_.widget_data_.find(opened_view.view()) !=
         root_pane_.widget_data_.end());
  root_pane_.widget_data_.erase(opened_view.view());

  return w;
}

OpenedView* ViewManagerWt::GetActiveView() {
  return nullptr;
}

void ViewManagerWt::SetViewTitle(OpenedView& opened_view,
                                 const std::wstring& title) {}

void ViewManagerWt::SaveLayout(PageLayout& layout) {}

void ViewManagerWt::SplitView(OpenedView& view, bool vertically) {
  if (view.window_info().is_pane())
    return;
}

void ViewManagerWt::AddView(OpenedView& view) {
  root_pane_.AddView(view);
}

void ViewManagerWt::RootPane::AddView(OpenedView& view) {
  if (view.window_info().is_pane()) {
    auto side =
        view.window_info().dock_bottom() ? DockSide::Bottom : DockSide::Left;
    GetOrCreateDockPane(side).AddView(view);
  } else {
    center_pane_.AddView(view);
  }
}

ViewManagerWt::DockPane& ViewManagerWt::RootPane::GetOrCreateDockPane(
    DockSide side) {
  auto& pane = dock_pane(side);
  if (!pane)
    pane = std::make_unique<DockPane>(view_manager_, *this, side);
  return *pane;
}

ViewManagerWt::Pane* ViewManagerWt::RootPane::FindWidgetPane(
    Wt::WWidget& widget) {
  auto i = widget_data_.find(&widget);
  return i != widget_data_.end() ? i->second.pane : nullptr;
}

ViewManagerWt::DockPane::DockPane(ViewManagerWt& view_manager,
                                  RootPane& root_pane,
                                  DockSide side)
    : view_manager_{view_manager}, root_pane_{root_pane}, side_{side} {
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

void ViewManagerWt::DockPane::AddView(OpenedView& view) {
  assert(view.view());
  assert(!view_manager_.IsViewAdded(view));

  // Open HardwareView to the bottom of opened view.
  const bool tabify = view.window_info().command_id != ID_HARDWARE_VIEW;

  DockSubPane* subpane = nullptr;
  if (!subpanes_.empty() && tabify) {
    subpane = &subpanes_.front();
  } else {
    subpane = &subpanes_.emplace_back(view_manager_, root_pane_, *this);
  }

  subpane->AddView(view);
}

ViewManagerWt::DockSubPane::DockSubPane(ViewManagerWt& view_manager,
                                        RootPane& root_pane,
                                        DockPane& dock_pane)
    : view_manager_{view_manager},
      root_pane_{root_pane},
      dock_pane_{dock_pane} {
  tab_widget_ = dock_pane.layout_->addWidget(root_pane_.CreateTabWidget(), 1);
  if (dock_pane.side_ == DockSide::Bottom)
    tab_widget_->setHeight(200);
  else
    tab_widget_->setWidth(200);
}

void ViewManagerWt::DockSubPane::AddView(OpenedView& view) {
  assert(view.view());
  assert(!view_manager_.IsViewAdded(view));

  auto* tab =
      tab_widget_->addTab(std::unique_ptr<Wt::WWidget>(view.view()),
                          view.GetWindowTitle(), Wt::ContentLoading::Eager);
  tab->setCloseable(true);

  /*view.view()->resize(Wt::WLength{100, Wt::LengthUnit::Percentage},
                      Wt::WLength{100, Wt::LengthUnit::Percentage});*/
  view.view()->setHeight(Wt::WLength{99, Wt::LengthUnit::Percentage});

  assert(root_pane_.widget_data_.find(view.view()) ==
         root_pane_.widget_data_.end());
  root_pane_.widget_data_.emplace(view.view(),
                                  RootPane::WidgetData{tab_widget_});

  view_manager_.added_views_.emplace_back(&view);
}

std::unique_ptr<ViewManagerWt::Block> ViewManagerWt::OpenLayoutBlock(
    const Page& page,
    const PageLayoutBlock& block) {
  if (block.type == PageLayoutBlock::PANE) {
    std::vector<OpenedView*> tab_views;
    for (int window_id : block.wins) {
      auto* opened_view = FindViewByID(window_id);
      if (!opened_view)
        continue;

      if (opened_view->window_info().is_pane()) {
        auto side = opened_view->window_info().dock_bottom() ? DockSide::Left
                                                             : DockSide::Bottom;
        root_pane_.GetOrCreateDockPane(side).AddView(*opened_view);
      } else {
        tab_views.emplace_back(opened_view);
      }
    }

    if (tab_views.empty())
      return nullptr;

    auto tab_widget = root_pane_.CreateTabWidget();
    for (auto* opened_view : tab_views) {
      if (!IsViewAdded(*opened_view)) {
        auto* tab = tab_widget->addTab(
            std::unique_ptr<Wt::WWidget>(opened_view->view()),
            opened_view->GetWindowTitle(), Wt::ContentLoading::Eager);
        tab->setCloseable(true);
        // tab->focussed().connect([this, tab] {
        // delegate_.OnActiveViewChanged(FindViewByWidget(tab)); });
        /*tab_widget->contentsStack()->focussed().connect([this, tab] {
          delegate_.OnActiveViewChanged(FindViewByWidget(tab));
        });*/
        added_views_.emplace_back(opened_view);
      }
    }
    return std::make_unique<Block>(std::move(tab_widget));

  } else if (block.type == PageLayoutBlock::SPLIT) {
    auto child1 = OpenLayoutBlock(page, *block.left);
    auto child2 = OpenLayoutBlock(page, *block.right);
    if (!child1)
      return child2;
    if (!child2)
      return child1;

    auto layout = block.horz ? static_cast<std::unique_ptr<Wt::WBoxLayout>>(
                                   Wt::cpp14::make_unique<Wt::WHBoxLayout>())
                             : static_cast<std::unique_ptr<Wt::WBoxLayout>>(
                                   Wt::cpp14::make_unique<Wt::WVBoxLayout>());
    child1->AddToLayout(*layout);
    child2->AddToLayout(*layout);
    // layout->setResizable(0);
    /*splitter->setSizes(
        {std::numeric_limits<int>::max() / 100 * block.pos,
         std::numeric_limits<int>::max() / 100 * (100 - block.pos)});*/
    return std::make_unique<Block>(std::move(layout));

  } else {
    assert(false);
    return nullptr;
  }
}

void ViewManagerWt::CenterPane::AddView(OpenedView& view) {
  assert(view.view());
  assert(!view_manager_.IsViewAdded(view));

  auto* tab_widget = view_manager_.active_view_
                         ? root_pane_.GetTabWidget(*view_manager_.active_view_)
                         : nullptr;
  if (!tab_widget)
    tab_widget = GetFirstTabWidget(*root_pane_.root_layout_);

  if (!tab_widget) {
    auto root_widget = root_pane_.CreateTabWidget();
    tab_widget = root_widget.get();
    root_pane_.SetRootBlock(std::make_unique<Block>(std::move(root_widget)));
  }

  auto* tab =
      tab_widget->addTab(std::unique_ptr<Wt::WWidget>(view.view()),
                         view.GetWindowTitle(), Wt::ContentLoading::Eager);
  tab->setCloseable(true);

  assert(root_pane_.widget_data_.find(view.view()) ==
         root_pane_.widget_data_.end());
  root_pane_.widget_data_.emplace(view.view(),
                                  RootPane::WidgetData{tab_widget, this});

  view_manager_.added_views_.emplace_back(&view);
}

std::unique_ptr<Wt::WTabWidget> ViewManagerWt::RootPane::CreateTabWidget() {
  auto tab_widget = std::make_unique<Wt::WTabWidget>();
  /*tabs->setDocumentMode(true);
  tabs->setMovable(true);
  tabs->setTabsClosable(true);*/
  tab_widget->setAttributeValue(sc_layoutWidgetTypeProp,
                                ToString(LayoutWidgetType::Tabs));

  tab_widget->tabClosed().connect([this, &tab_widget = *tab_widget](int index) {
    auto* opened_view =
        view_manager_.FindViewByWidget(tab_widget.widget(index));
    if (opened_view) {
      assert(tab_widget.indexOf(opened_view->view()) != -1);
      RemoveView(*opened_view);
    }
  });

  /*QObject::connect(
      tabs_ptr, &DockTabWidget::tabDropped, this,
      [this, tabs_ptr](DockTabWidget& source, int source_index,
                       DockTabWidget::DropSide side) {
        if (&source == tabs_ptr && side == DockTabWidget::DropSide::Center)
          return;

        auto* source_widget = source.widget(source_index);
        assert(source_widget);
        auto source_text = source.tabText(source_index);
        source.removeTab(source_index);

        auto* target = tabs_ptr;
        if (side != DockTabWidget::DropSide::Center)
          target = &SplitTabBlock(*tabs_ptr, side);

        int index = target->addTab(source_widget, source_text);
        target->setCurrentIndex(index);

        if (source.count() == 0)
          DeleteTabBlock(source, true);
      });*/

  /*auto tab_bar = tabs_ptr->tabBar();
  tab_bar->setContextMenuPolicy(Qt::CustomContextMenu);
  QObject::connect(
      tab_bar, &DockTabWidget::customContextMenuRequested, this,
      [this, tabs_ptr, tab_bar](const QPoint& pos) {
        int index = tab_bar->tabAt(pos);
        if (index == -1)
          return;
        auto* view = FindViewByWidget(tabs_ptr->widget(index));
        if (!view)
          return;
        auto global_pos = tab_bar->mapToGlobal(pos);
        delegate_.OnShowTabPopupMenu(*view, {global_pos.x(), global_pos.y()});
      });*/

  return tab_widget;
}

Wt::WTabWidget* ViewManagerWt::RootPane::GetTabWidget(OpenedView& opened_view) {
  auto i = widget_data_.find(opened_view.view());
  return i != widget_data_.end() ? i->second.tab_widget : nullptr;
}

Wt::WLayout* ViewManagerWt::RootPane::GetParentLayout(
    Wt::WTabWidget& tab_widget) {
  auto* parent = static_cast<Wt::WContainerWidget*>(tab_widget.parent());
  if (!parent)
    return nullptr;

  auto* layout = parent->layout();
  if (!layout)
    return nullptr;

  auto* item = layout->findWidgetItem(&tab_widget);
  if (!item)
    return nullptr;

  return item->parentLayout();
}

ViewManagerWt::RootPane::RootPane(ViewManagerWt& view_manager)
    : view_manager_{view_manager} {
  root_layout_ = new Wt::WBorderLayout();
  root_layout_->setContentsMargins(0, 0, 0, 0);
  root_layout_->addWidget(CreateTabWidget(), Wt::LayoutPosition::Center);
}
