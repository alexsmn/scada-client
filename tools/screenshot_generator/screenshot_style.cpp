#include "screenshot_style.h"

#include <QPainter>
#include <QPainterStateGuard>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleOption>

namespace scada::screenshot_generator {
namespace {

// Fusion with the idle line-edit frame drawn from QPalette::Mid.
class ScreenshotStyle : public QProxyStyle {
 public:
  ScreenshotStyle() : QProxyStyle{QStyleFactory::create("Fusion")} {
    // Keep the name the rest of the fixture and any style check expects.
    setObjectName(baseStyle()->objectName());
  }

  void drawPrimitive(PrimitiveElement element,
                     const QStyleOption* option,
                     QPainter* painter,
                     const QWidget* widget) const override {
    QProxyStyle::drawPrimitive(element, option, painter, widget);
    if (element != PE_FrameLineEdit || (option->state & State_HasFocus))
      return;
    // Restroke the outline Fusion just drew, with the same geometry
    // (QFusionStyle::drawPrimitive, PE_FrameLineEdit), so only its colour
    // changes and nothing moves by a pixel.
    QPainterStateGuard guard{painter};
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->translate(0.5, 0.5);
    painter->setPen(option->palette.color(QPalette::Mid));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(option->rect.adjusted(0, 0, -1, -1), 2, 2);
  }
};

}  // namespace

QStyle* MakeScreenshotStyle() {
  return new ScreenshotStyle;
}

}  // namespace scada::screenshot_generator
