#pragma once

#include "components/main/view_manager.h"

#include <QObject>

class QMainWindow;
class QTabWidget;
class QWidget;

class ViewManagerQt final : public QObject, public ViewManager {
  Q_OBJECT

 public:
  ViewManagerQt(QMainWindow& main_window, ViewManagerDelegate& delegate);
  ~ViewManagerQt();

  // ViewManager
  virtual OpenedView* GetActiveView() override;
  virtual void SetViewTitle(OpenedView& view,
                            const base::string16& title) override;
  virtual void ActivateView(OpenedView& view) override;
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

  std::unique_ptr<QTabWidget> CreateTabBlock();
  void DeleteTabBlock(QTabWidget& tabs);

  std::unique_ptr<QWidget> OpenLayoutBlock(const Page& page,
                                           const PageLayoutBlock& block);
  void SaveLayoutBlock(PageLayoutBlock& block, QWidget& widget);

  OpenedView* FindViewByWidget(const QWidget* widget);

  void OnFocusChanged(QObject* focus_object);

  QMainWindow& main_window_;
};
