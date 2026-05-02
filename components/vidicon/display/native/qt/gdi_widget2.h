#pragma once

#include "base/win/scoped_gdi_object.h"
#include "base/win/scoped_hdc.h"
#include "base/win/scoped_select_object.h"
#include "vidicon/display/native/qt/window_util.h"

#include <QGraphicsView>
#include <QPaintEvent>
#include <QPainter>
#include <QToolTip>
#include <Windows.h>
#include <qwinfunctions.h>

class GdiWidget2 : public QWidget {
 public:
  explicit GdiWidget2(QWidget* parent = nullptr) : QWidget{parent} {
    // For tooltips.
    setMouseTracking(true);

    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
  }

  virtual void paintEvent(QPaintEvent* event) override {
    QPainter painter{this};

    auto rect = painter.clipBoundingRect().toRect();
    if (rect.isNull()) {
      rect = QRect{0, 0, this->width(), this->height()};
    }

    base::win::ScopedGetDC desktop_dc{HWND_DESKTOP};
    base::win::ScopedCreateDC dc{::CreateCompatibleDC(desktop_dc)};
    base::win::ScopedBitmap bitmap{
        ::CreateCompatibleBitmap(desktop_dc, rect.width(), rect.height())};
    base::win::ScopedSelectObject select_bitmap{dc.Get(), bitmap.get()};

    auto paint_rect = ToRECT(rect);
    ::FillRect(dc.Get(), &paint_rect, (HBRUSH)(COLOR_WINDOW + 1));
    paint(dc.Get(), paint_rect);

    QPixmap tile = QtWin::fromHBITMAP(bitmap.get());
    painter.drawPixmap(rect.x(), rect.y(), tile);
  }

  virtual bool event(QEvent* event) override {
    if (event->type() == QEvent::ToolTip) {
      const QHelpEvent& help_event = *static_cast<QHelpEvent*>(event);
      if (auto tooltip = tooltipAt(help_event.pos()); !tooltip.isEmpty()) {
        QToolTip::showText(help_event.globalPos(), tooltip);
      } else {
        QToolTip::hideText();
        event->ignore();
      }
      return true;
    }
    return QWidget::event(event);
  }

 protected:
  virtual void paint(HDC dc, const RECT& rect) {}

  virtual QString tooltipAt(const QPoint& p) const { return {}; }
};
