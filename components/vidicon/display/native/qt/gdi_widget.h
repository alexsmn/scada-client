#pragma once

#include <QEvent>
#include <QGraphicsView>
#include <QPainter>
#include <Windows.h>
// #include <qwinfunctions.h>

class GdiWidget : public QGraphicsView {
 public:
  explicit GdiWidget(QWidget* parent = nullptr) : QGraphicsView{parent} {
    // setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setRenderHint(QPainter::Antialiasing);
  }

  virtual QPaintEngine* paintEngine() const override { return 0; }

  virtual bool event(QEvent* event) override {
    if (event->type() == QEvent::Paint) {
      //bool result = QGraphicsView::event(event);
      paintOverlay();
      return true;
    }
    if (event->type() == QEvent::UpdateRequest) {
      bool result = QGraphicsView::event(event);
      paintOverlay();
      return result;
    }
    return QGraphicsView::event(event);
  }

  virtual void paintEvent(QPaintEvent* event) override {
    // Suppress the default handler.
  }

  void paintOverlay() {
    // We're called after the native painter has done its thing
    HWND hwnd = (HWND)viewport()->winId();
    HDC hdc = GetDC(hwnd);

    RECT rect;
    GetClientRect(hwnd, &rect);

    paint(hdc, rect);

    ReleaseDC(hwnd, hdc);
  }

 protected:
  virtual void paint(HDC dc, const RECT& rect) {}
};
