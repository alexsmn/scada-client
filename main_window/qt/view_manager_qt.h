#pragma once

#include "aui/qt/dock_tab_widget.h"
#include "main_window/view_manager.h"

class QMainWindow;
class QWidget;

class ViewManagerQt final : public QObject, public ViewManager {
  Q_OBJECT

 public:
  ViewManagerQt(QMainWindow& main_window, ViewManagerDelegate& delegate);
  ~ViewManagerQt();

  // ViewManager
  virtual OpenedView* GetActiveView() override;
  virtual void SetViewTitle(OpenedView& view,
                            const std::u16string& title) override;
  virtual void ActivateView(const OpenedView& view) override;
  virtual void CloseView(OpenedView& view) override;
  virtual void SplitView(OpenedView& view, bool vertically) override;

 protected:
  // ViewManager
  virtual void OpenLayout(Page& page, const PageLayout& layout) override;
  virtual void SaveLayout(PageLayout& layout) override;
  virtual void AddView(OpenedView& view) override;

 private:
  void AddTabView(OpenedView& view);
  void AddDockView(OpenedView& view);

  std::unique_ptr<DockTabWidget> CreateTabBlock();
  void DeleteTabBlock(DockTabWidget& tabs, bool later);
  DockTabWidget& SplitTabBlock(DockTabWidget& tabs,
                               DockTabWidget::DropSide side);

  std::unique_ptr<QWidget> OpenLayoutBlock(const Page& page,
                                           const PageLayoutBlock& block);
  void SaveLayoutBlock(PageLayoutBlock& block, QWidget& widget);

  OpenedView* FindViewByWidget(const QWidget* widget);

  void OnFocusChanged(QObject* focus_object);

  void SetRootWidget(QWidget* widget);

  QMainWindow& main_window_;
  QWidget* root_widget_ = nullptr;
};
