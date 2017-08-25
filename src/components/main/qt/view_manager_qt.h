#pragma once

#include <QObject>

#include "client/components/main/view_manager.h"

class ClientApplicationQt;
class QMainWindow;
class QTabWidget;
class QWidget;

class ViewManagerQt : public ViewManager {
 public:
  ViewManagerQt(ClientApplicationQt& app, QMainWindow& main_window, ViewManagerDelegate* delegate);

  // ViewManager
  virtual void OpenLayout(Page& page, const PageLayout& layout) override;
  virtual void SaveLayout(PageLayout& layout) override;
  virtual void ActivateView(OpenedView& view) override;
  virtual void CloseView(OpenedView& view) override;
  virtual void SetViewTitle(OpenedView& view, const base::string16& title) override;

 protected:
  void AddTabView(OpenedView& view);
  void AddDockView(OpenedView& view);

  std::unique_ptr<QTabWidget> CreateTabBlock();

  std::unique_ptr<QWidget> OpenLayoutBlock(const Page& page, const PageLayoutBlock& block);
  void SaveLayoutBlock(PageLayoutBlock& block, QWidget& widget);

  OpenedView* FindViewByWidget(const QWidget* widget);

  void OnFocusChanged();
  void OnViewCloseRequested(OpenedView& opened_view);

  // ViewManager
  virtual void AddView(OpenedView& view) override;

 private:
  QMainWindow& main_window_;

  QMetaObject::Connection focused_changed_connection_;
};
