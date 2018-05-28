#include "components/main/qt/view_manager_qt.h"

#include "base/auto_reset.h"
#include "common_resources.h"
#include "components/main/opened_view.h"
#include "services/page.h"
#include "window_info.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QMainWindow>
#include <QSplitter>

namespace {

class CustomDockWidget : public QDockWidget {
 public:
  CustomDockWidget(QWidget* parent = nullptr) : QDockWidget(parent) {}

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

QDockWidget* GetDockWidget(OpenedView& opened_view) {
  return opened_view.window_info().is_pane()
             ? static_cast<QDockWidget*>(opened_view.view()->parentWidget())
             : nullptr;
}

QTabWidget* GetTabWidget(OpenedView& opened_view) {
  if (opened_view.window_info().is_pane())
    return nullptr;
  if (!opened_view.view())
    return nullptr;
  auto* parent = opened_view.view()->parentWidget();
  if (!parent)
    return nullptr;
  return static_cast<QTabWidget*>(parent->parentWidget());
}

QTabWidget* GetFirstTabBlock(QWidget& widget) {
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
    return &static_cast<QTabWidget&>(widget);

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

ViewManagerQt::ViewManagerQt(QMainWindow& main_window,
                             ViewManagerDelegate& delegate)
    : ViewManager{delegate}, main_window_{main_window} {
  QObject::connect(static_cast<QGuiApplication*>(QApplication::instance()),
                   &QGuiApplication::focusObjectChanged, this,
                   &ViewManagerQt::OnFocusChanged);
}

ViewManagerQt::~ViewManagerQt() {}

void ViewManagerQt::OpenLayout(Page& page, const PageLayout& layout) {
  main_window_.setCentralWidget(
      OpenLayoutBlock(page, page.layout.main).release());

  // Open windows not opended by layout.
  for (auto* opened_view : views_) {
    if (!IsViewAdded(*opened_view))
      AddView(*opened_view);
  }

  if (!layout.blob.empty()) {
    ScopedNames names{views_};
    main_window_.restoreState(QByteArray::fromStdString(layout.blob));
  }
}

std::unique_ptr<QTabWidget> ViewManagerQt::CreateTabBlock() {
  auto tabs = std::make_unique<QTabWidget>();
  tabs->setDocumentMode(true);
  tabs->setMovable(true);
  tabs->setTabsClosable(true);
  tabs->setProperty(sc_layoutWidgetTypeProp,
                    static_cast<int>(LayoutWidgetType::Tabs));
  auto* tabs_ptr = tabs.get();
  QObject::connect(tabs.get(), &QTabWidget::tabCloseRequested, this,
                   [this, tabs_ptr](int index) {
                     auto* opened_view =
                         FindViewByWidget(tabs_ptr->widget(index));
                     if (opened_view)
                       CloseView(*opened_view);
                   });
  return tabs;
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
                     QString::fromStdWString(opened_view->GetWindowTitle()));
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
    splitter->setStretchFactor(1, block.pos);
    return std::move(splitter);

  } else {
    assert(false);
    return nullptr;
  }
}

OpenedView* ViewManagerQt::FindViewByWidget(const QWidget* widget) {
  auto i = std::find_if(views_.begin(), views_.end(),
                        [widget](OpenedView* opened_view) {
                          return opened_view->view() == widget;
                        });
  return i == views_.end() ? nullptr : *i;
}

void ViewManagerQt::OnFocusChanged() {
  SetActiveView(GetActiveView());
}

void ViewManagerQt::ActivateView(OpenedView& opened_view) {
  if (auto* tabs = GetTabWidget(opened_view)) {
    auto index = tabs->indexOf(opened_view.view());
    if (index != -1)
      tabs->setCurrentIndex(index);

  } else if (auto* dock = GetDockWidget(opened_view)) {
    // Detach from parent to avoid deletion by dock.
  }

  opened_view.view()->setFocus(Qt::ActiveWindowFocusReason);
}

void ViewManagerQt::CloseView(OpenedView& opened_view) {
  if (auto* tabs = GetTabWidget(opened_view)) {
    auto index = tabs->indexOf(opened_view.view());
    if (index != -1)
      tabs->removeTab(index);
    // TODO: Remove tab widget.

  } else if (auto* dock = GetDockWidget(opened_view)) {
    // Detach from parent to avoid deletion by dock.
    opened_view.view()->setParent(nullptr);
    delete dock;
  }

  DestroyView(opened_view);
}

OpenedView* ViewManagerQt::GetActiveView() {
  return FindViewByWidget(QApplication::focusWidget());
}

void ViewManagerQt::SetViewTitle(OpenedView& opened_view,
                                 const base::string16& title) {
  if (auto* tabs = GetTabWidget(opened_view)) {
    auto index = tabs->indexOf(opened_view.view());
    if (index != -1)
      tabs->setTabText(index, QString::fromStdWString(title));

  } else if (auto* dock = GetDockWidget(opened_view)) {
    dock->setWindowTitle(QString::fromStdWString(title));
  }
}

void ViewManagerQt::SaveLayout(PageLayout& layout) {
  if (main_window_.centralWidget())
    SaveLayoutBlock(layout.main, *main_window_.centralWidget());

  ScopedNames names{views_};
  layout.blob = main_window_.saveState().toStdString();
}

void ViewManagerQt::SaveLayoutBlock(PageLayoutBlock& block, QWidget& widget) {
  auto type_prop = widget.property(sc_layoutWidgetTypeProp);
  assert(!type_prop.isNull());

  auto type = static_cast<LayoutWidgetType>(type_prop.toInt());
  if (type == LayoutWidgetType::Splitter) {
    auto& splitter = static_cast<QSplitter&>(widget);
    block.split(splitter.orientation() == Qt::Vertical);
    block.pos = 50;
    if (auto* left = splitter.widget(0))
      SaveLayoutBlock(*block.left, *left);
    if (auto* right = splitter.widget(1))
      SaveLayoutBlock(*block.right, *right);

  } else if (type == LayoutWidgetType::Tabs) {
    auto& tabs = static_cast<QTabWidget&>(widget);
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
  dock->setWindowTitle(QString::fromStdWString(view.GetWindowTitle()));
  dock->setWidget(view.view());
  dock->SetClosedHandler([this, &view] { CloseView(view); });

  QDockWidget* tabify_to = nullptr;
  // Open HardwareView to the bottom of opened view.
  if (view.window_info().command_id != ID_HARDWARE_VIEW) {
    for (auto* dock : main_window_.findChildren<CustomDockWidget*>()) {
      if (main_window_.dockWidgetArea(dock) == Qt::LeftDockWidgetArea) {
        tabify_to = dock;
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
  if (!tabs && main_window_.centralWidget())
    tabs = GetFirstTabBlock(*main_window_.centralWidget());
  if (!tabs) {
    tabs = CreateTabBlock().release();
    main_window_.setCentralWidget(tabs);
  }
  tabs->addTab(view.view(), QString::fromStdWString(view.GetWindowTitle()));

  added_views_.emplace_back(&view);
}
