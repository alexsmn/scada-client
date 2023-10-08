#include "components/main/wt/view_manager2_wt.h"

#include "base/auto_reset.h"
#include "common_resources.h"
#include "components/main/opened_view.h"
#include "components/main/view_manager_delegate.h"
#include "services/page.h"
#include "controller/window_info.h"

#pragma warning(push)
#pragma warning(disable : 4251 4275)
#include <wt/WApplication.h>
#include <wt/WBorderLayout.h>
#include <wt/WContainerWidget.h>
#include <wt/WHBoxLayout.h>
#include <wt/WMenuItem.h>
#include <wt/WStackedWidget.h>
#include <wt/WTabWidget.h>
#include <wt/WVBoxLayout.h>
#include <wt/WWidgetItem.h>
#pragma warning(pop)

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

// ViewManager2Wt

ViewManager2Wt::ViewManager2Wt(ViewManagerDelegate& delegate)
    : ViewManager{delegate} {
  root_layout_ = new Wt::WBorderLayout();
  root_layout_->setContentsMargins(0, 0, 0, 0);
  center_tab_widget_ =
      root_layout_->addWidget(CreateTabWidget(), Wt::LayoutPosition::Center);
}

ViewManager2Wt::~ViewManager2Wt() {}

Wt::WLayout& ViewManager2Wt::root_layout() {
  assert(root_layout_);
  return *root_layout_;
}

void ViewManager2Wt::OpenLayout(Page& page, const PageLayout& layout) {
  Wt::WApplication::UpdateLock update_lock{Wt::WApplication::instance()};

  OpenLayoutBlock(page, layout.main);

  // Open windows not opended by layout.
  for (auto* opened_view : views_) {
    if (!IsViewAdded(*opened_view))
      AddView(*opened_view);
  }
}

OpenedView* ViewManager2Wt::FindViewByWidget(const Wt::WWidget* widget) {
  auto i = std::find_if(views_.begin(), views_.end(),
                        [widget](OpenedView* opened_view) {
                          return opened_view->view() == widget;
                        });
  return i != views_.end() ? *i : nullptr;
}

void ViewManager2Wt::ActivateView(OpenedView& opened_view) {}

void ViewManager2Wt::CloseView(OpenedView& opened_view) {
  // TODO:
}

OpenedView* ViewManager2Wt::GetActiveView() {
  return nullptr;
}

void ViewManager2Wt::SetViewTitle(OpenedView& opened_view,
                                  const std::u16string& title) {}

void ViewManager2Wt::SaveLayout(PageLayout& layout) {}

void ViewManager2Wt::SplitView(OpenedView& view, bool vertically) {
  if (view.window_info().is_pane())
    return;
}

void ViewManager2Wt::AddView(OpenedView& view) {}

void ViewManager2Wt::OpenLayoutBlock(const Page& page,
                                     const PageLayoutBlock& block) {
  /*if (block.type == PageLayoutBlock::PANE) {
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
            opened_view->GetWindowTitle());
        tab->setCloseable(true);
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
    return std::make_unique<Block>(std::move(layout));

  } else {
    assert(false);
    return nullptr;
  }*/
}

std::unique_ptr<Wt::WTabWidget> ViewManager2Wt::CreateTabWidget() {
  auto tab_widget = std::make_unique<Wt::WTabWidget>();
  /*tabs->setDocumentMode(true);
  tabs->setMovable(true);
  tabs->setTabsClosable(true);*/
  tab_widget->setAttributeValue(sc_layoutWidgetTypeProp,
                                ToString(LayoutWidgetType::Tabs));

  tab_widget->tabClosed().connect([this, &tab_widget = *tab_widget](int index) {
    /*auto* opened_view =
        view_manager_.FindViewByWidget(tab_widget.widget(index));
    if (opened_view)
      RemoveView(*opened_view);*/
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
