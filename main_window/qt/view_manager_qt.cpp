#include "main_window/qt/view_manager_qt.h"

#include "aui/qt/client_utils_qt.h"
#include "base/auto_reset.h"
#include "common_resources.h"
#include "controller/window_info.h"
#include "main_window/opened_view.h"
#include "main_window/view_manager_delegate.h"
#include "profile/page.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QSplitter>

namespace {

class CustomDockWidget : public QDockWidget {
 public:
  explicit CustomDockWidget(QWidget* parent = nullptr) : QDockWidget(parent) {}

  typedef std::function<void()> ClosedHandler;
  void SetClosedHandler(ClosedHandler handler) {
    closed_handler_ = std::move(handler);
  }

 protected:
  void closeEvent(QCloseEvent* e) override {
    if (closed_handler_)
      closed_handler_();
    e->accept();
  }

 private:
  ClosedHandler closed_handler_;
};

const char sc_layoutWidgetTypeProp[] = "LayoutWidgetType";

enum class LayoutWidgetType { Splitter, Tabs, Window, Panel };

QDockWidget* GetDockWidget(const OpenedView& view) {
  return view.window_info().is_pane()
             ? static_cast<QDockWidget*>(view.view()->parentWidget())
             : nullptr;
}

DockTabWidget* GetTabWidget(const OpenedView& view) {
  if (view.window_info().is_pane() || !view.view()) {
    return nullptr;
  }

  QWidget* parent = view.view()->parentWidget();
  if (!parent) {
    return nullptr;
  }

  return static_cast<DockTabWidget*>(parent->parentWidget());
}

DockTabWidget* GetFirstTabBlock(QWidget& widget) {
  auto type_prop = widget.property(sc_layoutWidgetTypeProp);
  assert(!type_prop.isNull());

  auto type = static_cast<LayoutWidgetType>(type_prop.toInt());
  if (type == LayoutWidgetType::Splitter) {
    auto& splitter = static_cast<QSplitter&>(widget);
    if (auto* left = splitter.widget(0))
      return GetFirstTabBlock(*left);
    else if (auto* right = splitter.widget(1))
      return GetFirstTabBlock(*right);
    else
      return nullptr;

  } else if (type == LayoutWidgetType::Tabs) {
    return &static_cast<DockTabWidget&>(widget);

  } else {
    return nullptr;
  }
}

struct ScopedNames {
  explicit ScopedNames(const std::list<OpenedView*>& views) : views_{views} {
    for (auto* opened_view : views_) {
      if (auto* dock = GetDockWidget(*opened_view))
        dock->setObjectName(QString{"dock-%1"}.arg(opened_view->window_id()));
    }
  }

  ~ScopedNames() {
    for (auto* opened_view : views_) {
      if (auto* dock = GetDockWidget(*opened_view))
        dock->setObjectName({});
    }
  }

 private:
  const std::list<OpenedView*>& views_;
};

}  // namespace

// ViewManagerQt

ViewManagerQt::ViewManagerQt(QMainWindow& main_window,
                             ViewManagerDelegate& delegate)
    : ViewManager{delegate}, main_window_{main_window} {
  auto* central_widget = new QWidget;
  central_widget->setLayout(new QHBoxLayout);
  central_widget->layout()->setMargin(0);
  main_window_.setCentralWidget(central_widget);

  QObject::connect(static_cast<QGuiApplication*>(QApplication::instance()),
                   &QGuiApplication::focusObjectChanged, this,
                   &ViewManagerQt::OnFocusChanged);
}

ViewManagerQt::~ViewManagerQt() {}

void ViewManagerQt::OpenLayout(Page& page, const PageLayout& layout) {
  SetRootWidget(OpenLayoutBlock(page, page.layout.main).release());

  // Open windows not opended by layout.
  for (auto* opened_view : views_) {
    if (!IsViewAdded(*opened_view))
      AddView(*opened_view);
  }

  if (!layout.blob.empty()) {
    ScopedNames names{views_};
    main_window_.restoreState(
        QByteArray::fromRawData(layout.blob.data(), layout.blob.size()));
  }
}

std::unique_ptr<DockTabWidget> ViewManagerQt::CreateTabBlock() {
  auto tabs = std::make_unique<DockTabWidget>();
  tabs->setDocumentMode(true);
  tabs->setMovable(true);
  tabs->setTabsClosable(true);
  tabs->setProperty(sc_layoutWidgetTypeProp,
                    static_cast<int>(LayoutWidgetType::Tabs));

  auto* tabs_ptr = tabs.get();

  QObject::connect(tabs_ptr, &DockTabWidget::tabCloseRequested, this,
                   [this, tabs_ptr](int index) {
                     auto* opened_view =
                         FindViewByWidget(tabs_ptr->widget(index));
                     if (opened_view)
                       CloseView(*opened_view);
                   });

  QObject::connect(
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
      });

  auto tab_bar = tabs_ptr->tabBar();
  tab_bar->setContextMenuPolicy(Qt::CustomContextMenu);
  QObject::connect(tab_bar, &DockTabWidget::customContextMenuRequested, this,
                   [this, tabs_ptr, tab_bar](const QPoint& pos) {
                     int index = tab_bar->tabAt(pos);
                     if (index == -1)
                       return;
                     auto* view = FindViewByWidget(tabs_ptr->widget(index));
                     if (!view)
                       return;
                     auto global_pos = tab_bar->mapToGlobal(pos);
                     delegate_.OnShowTabPopupMenu(*view, global_pos);
                   });

  return tabs;
}

void ViewManagerQt::DeleteTabBlock(DockTabWidget& tabs, bool later) {
  assert(tabs.count() == 0);
  assert(&tabs != root_widget_);

  auto* splitter = qobject_cast<QSplitter*>(tabs.parentWidget());
  assert(splitter);

  int index = splitter->indexOf(&tabs);
  assert(index != -1);

  int other_index = (index + 1) % 2;
  auto* other_widget = splitter->widget(other_index);
  assert(other_widget);

  if (later) {
    tabs.setParent(nullptr);
    tabs.deleteLater();
  }

  if (splitter == root_widget_) {
    // Deletes |splitter| and |tabs|, owns |other_widget|.
    SetRootWidget(other_widget);

  } else {
    auto* parent_splitter = qobject_cast<QSplitter*>(splitter->parentWidget());
    assert(parent_splitter);

    int splitter_index = parent_splitter->indexOf(splitter);
    assert(splitter_index != -1);

    // Releases |splitter| ownership and owns |other_widget|.
    parent_splitter->replaceWidget(splitter_index, other_widget);
    // Deletes |splitter| and |tabs|.
    delete splitter;
  }
}

std::unique_ptr<QWidget> ViewManagerQt::OpenLayoutBlock(
    const Page& page,
    const PageLayoutBlock& block) {
  if (block.type == PageLayoutBlock::PANE) {
    std::vector<OpenedView*> tab_views;
    for (int window_id : block.wins) {
      auto* opened_view = FindViewByID(window_id);
      if (!opened_view)
        continue;

      if (opened_view->window_info().is_pane())
        AddDockView(*opened_view);
      else
        tab_views.emplace_back(opened_view);
    }

    if (tab_views.empty())
      return nullptr;

    auto tabs = CreateTabBlock();
    for (auto* opened_view : tab_views) {
      if (!IsViewAdded(*opened_view)) {
        tabs->addTab(opened_view->view(),
                     QString::fromStdU16String(opened_view->GetWindowTitle()));
        added_views_.emplace_back(opened_view);
      }
    }
    return std::move(tabs);

  } else if (block.type == PageLayoutBlock::SPLIT) {
    auto widget1 = OpenLayoutBlock(page, *block.left);
    auto widget2 = OpenLayoutBlock(page, *block.right);
    if (!widget1)
      return widget2;
    if (!widget2)
      return widget1;

    auto splitter = std::make_unique<QSplitter>();
    splitter->setOrientation(block.horz ? Qt::Vertical : Qt::Horizontal);
    splitter->setProperty(sc_layoutWidgetTypeProp,
                          static_cast<int>(LayoutWidgetType::Splitter));
    splitter->addWidget(widget1.release());
    splitter->addWidget(widget2.release());
    splitter->setSizes(
        {std::numeric_limits<int>::max() / 100 * block.pos,
         std::numeric_limits<int>::max() / 100 * (100 - block.pos)});
    return std::move(splitter);

  } else {
    assert(false);
    return nullptr;
  }
}

OpenedView* ViewManagerQt::FindViewByWidget(const QWidget* widget) {
  auto i = std::ranges::find_if(views_, [widget](const OpenedView* view) {
    return view->view() && view->view()->isAncestorOf(widget);
  });
  return i != views_.end() ? *i : nullptr;
}

void ViewManagerQt::OnFocusChanged(QObject* focus_object) {
  if (auto* view = GetActiveView())
    SetActiveView(view);
}

void ViewManagerQt::ActivateView(const OpenedView& view) {
  if (auto* tabs = GetTabWidget(view)) {
    auto index = tabs->indexOf(view.view());
    assert(index != -1);
    tabs->setCurrentIndex(index);

  } else if (auto* dock = GetDockWidget(view)) {
    // Detach from parent to avoid deletion by dock.
  }

  view.view()->setFocus(Qt::ActiveWindowFocusReason);
}

void ViewManagerQt::CloseView(OpenedView& view) {
  // WARNING: `GetTabWidget` requires `opened_view.view()` to be set.
  if (auto* tabs = GetTabWidget(view)) {
    auto index = tabs->indexOf(view.view());
    assert(index != -1);
    // Doesn't delete |view.view()|.
    tabs->removeTab(index);

    // Must reset the parent, or the view will be deleted by the tab widget.
    // While the view must be only deleted by the `OpenedView`.
    view.view()->setParent(nullptr);

    // Remove tab widget.
    if (tabs->count() == 0 && tabs != root_widget_) {
      DeleteTabBlock(*tabs, false);
    }

  } else if (auto* dock = GetDockWidget(view)) {
    // Can't delete immediately, since it's called by close event handler that
    // expects the widget to exist after the handler is processed.
    dock->deleteLater();
  }

  DestroyView(view);
}

OpenedView* ViewManagerQt::GetActiveView() {
  // WARNING: Can't use |focusWidget()| because it updates after
  // |focusObjectChanged()| signaled.
  return FindViewByWidget(qobject_cast<QWidget*>(QApplication::focusObject()));
}

void ViewManagerQt::SetViewTitle(OpenedView& view,
                                 const std::u16string& title) {
  if (auto* tabs = GetTabWidget(view)) {
    if (auto index = tabs->indexOf(view.view()); index != -1) {
      tabs->setTabText(index, QString::fromStdU16String(title));
    }

  } else if (auto* dock = GetDockWidget(view)) {
    dock->setWindowTitle(QString::fromStdU16String(title));
  }
}

void ViewManagerQt::SaveLayout(PageLayout& layout) {
  if (root_widget_)
    SaveLayoutBlock(layout.main, *root_widget_);

  ScopedNames names{views_};
  auto blob = main_window_.saveState().toStdString();
  layout.blob.assign(blob.begin(), blob.end());
}

void ViewManagerQt::SaveLayoutBlock(PageLayoutBlock& block, QWidget& widget) {
  auto type_prop = widget.property(sc_layoutWidgetTypeProp);
  assert(!type_prop.isNull());

  auto type = static_cast<LayoutWidgetType>(type_prop.toInt());
  if (type == LayoutWidgetType::Splitter) {
    auto& splitter = static_cast<QSplitter&>(widget);
    block.split(splitter.orientation() == Qt::Vertical);
    auto sizes = splitter.sizes();
    assert(sizes.size() == 2);
    block.pos = sizes[0] * 100 / (sizes[0] + sizes[1]);
    // block.pos = !sizes.empty() ? sizes.front() : 50;
    if (auto* left = splitter.widget(0))
      SaveLayoutBlock(*block.left, *left);
    if (auto* right = splitter.widget(1))
      SaveLayoutBlock(*block.right, *right);

  } else if (type == LayoutWidgetType::Tabs) {
    auto& tabs = static_cast<DockTabWidget&>(widget);
    for (int i = 0; i < tabs.count(); ++i) {
      auto* opened_view = FindViewByWidget(tabs.widget(i));
      if (opened_view)
        block.add(opened_view->window_id());
    }
  }
}

void ViewManagerQt::AddView(OpenedView& view) {
  if (view.window_info().is_pane())
    AddDockView(view);
  else
    AddTabView(view);
}

void ViewManagerQt::AddDockView(OpenedView& view) {
  assert(view.view());
  assert(!IsViewAdded(view));

  auto area = view.window_info().dock_bottom() ? Qt::BottomDockWidgetArea
                                               : Qt::LeftDockWidgetArea;
  auto* dock = new CustomDockWidget(&main_window_);
  dock->setWindowTitle(QString::fromStdU16String(view.GetWindowTitle()));
  dock->setWidget(view.view());
  dock->SetClosedHandler([this, &view] { CloseView(view); });

  QDockWidget* tabify_to = nullptr;
  // Open HardwareView to the bottom of opened view.
  if (view.window_info().command_id != ID_HARDWARE_VIEW) {
    for (auto* opened_dock : main_window_.findChildren<CustomDockWidget*>()) {
      if (main_window_.dockWidgetArea(opened_dock) == area) {
        tabify_to = opened_dock;
        break;
      }
    }
  }

  if (tabify_to)
    main_window_.tabifyDockWidget(tabify_to, dock);
  else
    main_window_.addDockWidget(area, dock);

  added_views_.emplace_back(&view);
}

void ViewManagerQt::AddTabView(OpenedView& view) {
  assert(view.view());
  assert(!IsViewAdded(view));

  auto* tabs = active_view_ ? GetTabWidget(*active_view_) : nullptr;
  if (!tabs && root_widget_)
    tabs = GetFirstTabBlock(*root_widget_);

  if (!tabs) {
    tabs = CreateTabBlock().release();
    SetRootWidget(tabs);
  }

  tabs->addTab(view.view(), QString::fromStdU16String(view.GetWindowTitle()));

  added_views_.emplace_back(&view);
}

void ViewManagerQt::SplitView(OpenedView& view, bool vertically) {
  if (view.window_info().is_pane()) {
    return;
  }

  auto* tabs = GetTabWidget(view);
  if (!tabs || tabs->count() < 2) {
    return;
  }

  // Doesn't delete the view.
  auto tab_index = tabs->indexOf(view.view());
  assert(tab_index != -1);
  tabs->removeTab(tab_index);

  auto side = vertically ? DockTabWidget::DropSide::Right
                         : DockTabWidget::DropSide::Bottom;
  auto& new_tabs = SplitTabBlock(*tabs, side);
  new_tabs.addTab(view.view(),
                  QString::fromStdU16String(view.GetWindowTitle()));
}

DockTabWidget& ViewManagerQt::SplitTabBlock(DockTabWidget& tabs,
                                            DockTabWidget::DropSide side) {
  assert(side != DockTabWidget::DropSide::Center);

  auto* splitter = new QSplitter;
  splitter->setProperty(sc_layoutWidgetTypeProp,
                        static_cast<int>(LayoutWidgetType::Splitter));
  splitter->setOrientation((side == DockTabWidget::DropSide::Top ||
                            side == DockTabWidget::DropSide::Bottom)
                               ? Qt::Vertical
                               : Qt::Horizontal);
  splitter->setChildrenCollapsible(false);

  if (&tabs == root_widget_) {
    // Reset ownership.
    tabs.setParent(nullptr);
    root_widget_ = nullptr;
    SetRootWidget(splitter);

  } else {
    auto* parent_splitter = qobject_cast<QSplitter*>(tabs.parentWidget());
    assert(parent_splitter);
    int parent_index = parent_splitter->indexOf(&tabs);
    assert(parent_index != -1);
    // Doesn't delete the splitter.
    parent_splitter->replaceWidget(parent_index, splitter);
  }

  auto* new_tabs = CreateTabBlock().release();

  if (side == DockTabWidget::DropSide::Right ||
      side == DockTabWidget::DropSide::Bottom) {
    splitter->addWidget(&tabs);
    splitter->addWidget(new_tabs);
  } else {
    splitter->addWidget(new_tabs);
    splitter->addWidget(&tabs);
  }

  return *new_tabs;
}

void ViewManagerQt::SetRootWidget(QWidget* widget) {
  if (root_widget_ == widget)
    return;

  if (root_widget_) {
    // Prevent from deletion if was hosted by |root_widget|.
    if (widget)
      widget->setParent(nullptr);

    delete root_widget_;
    root_widget_ = nullptr;
  }

  root_widget_ = widget;

  if (root_widget_)
    main_window_.centralWidget()->layout()->addWidget(root_widget_);
}
