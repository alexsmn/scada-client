#include "display_view/qt/display_widget.h"

#include "aui/qt/theme_qt.h"
#include "aui/translation.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPen>

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

using scada::display::RectF;
using scada::display::view::DisplayDocument;
using scada::display::view::DocumentKind;
using scada::display::view::Error;
using scada::display::view::ShapeHit;

// The selection halo, in widget pixels, from the mockup's SVG: a solid rect on
// the symbol bounds and a dashed one 6px outside it.
// docs/product/ui-mockups/screens/substation-display.html draws exactly this
// pair around the selected breaker (a 30x30 rect at 2.5px, a 42x42 at 1.5px
// with a 4-3 dash and .8 opacity).
//
// Widget pixels rather than page units on purpose: the halo is chrome, so it
// keeps its weight as the operator zooms, the way a focus ring does. A page-
// unit halo would thin out to nothing at Fit on a large schematic.
constexpr double kSelectionInnerWidth = 2.5;
constexpr double kSelectionOuterWidth = 1.5;
constexpr double kSelectionOuterMargin = 6.0;
constexpr double kSelectionOuterOpacity = 0.8;

QString Tr(std::string_view text) {
  return QString::fromStdU16String(Translate(text));
}

int SizeHintDimension(double value, int fallback) {
  if (!std::isfinite(value) || value <= 0)
    return fallback;
  return static_cast<int>(std::clamp(std::ceil(value), 1.0, 8192.0));
}

}  // namespace

QRectF DisplayPageRectToWidget(const RectF& page_rect,
                               const RectF& page_bounds,
                               QSize widget_size) {
  if (!std::isfinite(page_bounds.w) || !std::isfinite(page_bounds.h) ||
      page_bounds.w <= 0 || page_bounds.h <= 0 || widget_size.width() <= 0 ||
      widget_size.height() <= 0) {
    return {};
  }
  if (!std::isfinite(page_rect.x) || !std::isfinite(page_rect.y) ||
      !std::isfinite(page_rect.w) || !std::isfinite(page_rect.h)) {
    return {};
  }

  const double scale_x = widget_size.width() / page_bounds.w;
  const double scale_y = widget_size.height() / page_bounds.h;

  // The page rect's own top edge is the one with the LARGER page y, because
  // page y grows upward. Mapping both edges and taking the span keeps that
  // straight whatever sign the height has.
  const double top = (page_bounds.h - (page_rect.y + page_rect.h)) * scale_y;
  const double bottom = (page_bounds.h - page_rect.y) * scale_y;

  return QRectF{QPointF{page_rect.x * scale_x, std::min(top, bottom)},
                QSizeF{page_rect.w * scale_x, std::abs(bottom - top)}};
}

QString DisplayShapeLabel(const ShapeHit& hit) {
  if (!hit.name.empty())
    return QString::fromStdString(hit.name);
  return QString::fromStdString(hit.text);
}

DisplayWidget::DisplayWidget(QWidget* parent) : QWidget{parent} {
  setFocusPolicy(Qt::ClickFocus);
}

DisplayWidget::~DisplayWidget() = default;

bool DisplayWidget::Open(const std::filesystem::path& path, DocumentKind kind) {
  path_ = path;
  title_.clear();
  error_message_.clear();
  document_.reset();
  // A selection is a position in the OLD document and means nothing in the
  // new one, so it goes before the load rather than being left to paint over
  // whatever lands at those coordinates.
  SetSelection(std::nullopt);

  // A window definition that names no document resolves to the displays folder
  // itself. Handing that to the parser produced developer text about an
  // unsupported empty file extension, naming a directory — nothing an operator
  // can act on. Diagnose it here instead, before the parser ever sees it.
  std::error_code ec;
  if (path.empty() || std::filesystem::is_directory(path, ec)) {
    error_message_ = Tr("No display document is assigned to this window.");
    update();
    return false;
  }

  Error error;
  document_ = DisplayDocument::Open(path, kind, &error);
  if (!document_) {
    error_message_ = QStringLiteral("%1: %2").arg(
        Tr("Cannot open document"), QString::fromStdString(error.message));
    update();
    return false;
  }

  title_ = QString::fromStdString(document_->Title());
  updateGeometry();
  update();
  return true;
}

QSize DisplayWidget::sizeHint() const {
  if (!document_)
    return {640, 480};

  const RectF bounds = document_->PageBounds();
  return {SizeHintDimension(bounds.w, 640), SizeHintDimension(bounds.h, 480)};
}

void DisplayWidget::paintEvent(QPaintEvent*) {
  QPainter painter{this};

  if (!document_) {
    PaintMessage(painter, error_message_.isEmpty() ? Tr("No display is loaded.")
                                                   : error_message_);
    return;
  }

  if (width() <= 0 || height() <= 0)
    return;

  // Straight into the widget's painter: no intermediate image, which is the
  // whole point of linking the renderer rather than loading it.
  Error error;
  if (!document_->Render(painter,
                         RectF{0, 0, static_cast<double>(width()),
                               static_cast<double>(height())},
                         &error)) {
    PaintMessage(painter, QStringLiteral("%1: %2").arg(
                              Tr("Cannot render document"),
                              QString::fromStdString(error.message)));
    return;
  }

  // After the document, never before: the halo says which of the drawn shapes
  // is selected, so the drawing has to be underneath it.
  PaintSelection(painter);
}

void DisplayWidget::mousePressEvent(QMouseEvent* event) {
  setFocus(Qt::MouseFocusReason);
  if (!document_)
    return;

  const QPointF page = WidgetToPage(event->pos());
  if (!std::isfinite(page.x()) || !std::isfinite(page.y()))
    return;

  const std::optional<ShapeHit> hit = document_->HitTest({page.x(), page.y()});

  // A click on bare page clears the selection. That is the half the old code
  // had no way to express: it reported hits only, so the halo and the status
  // cell would have had no way back to "nothing selected" short of closing
  // the display.
  SetSelection(hit);

  if (hit)
    event->accept();
}

void DisplayWidget::mouseDoubleClickEvent(QMouseEvent* event) {
  if (double_click_callback_) {
    double_click_callback_();
    event->accept();
  }
}

void DisplayWidget::PaintMessage(QPainter& painter,
                                 const QString& message) const {
  painter.fillRect(rect(), QColor{255, 255, 255});
  painter.setPen(QColor{160, 0, 0});
  painter.drawText(rect().adjusted(24, 24, -24, -24),
                   Qt::AlignCenter | Qt::TextWordWrap, message);
}

void DisplayWidget::PaintSelection(QPainter& painter) const {
  if (!selection_ || !document_)
    return;

  const QRectF inner = DisplayPageRectToWidget(selection_->bounds,
                                               document_->PageBounds(), size());
  if (inner.isEmpty())
    return;

  // The accent token, the same one the rest of the workbench uses for "this is
  // the thing you have chosen". The mockup is explicit that selection belongs
  // to the design system rather than to the authored drawing, so it is read
  // from the theme here and never taken from the document.
  const QColor accent = scada::aui::ActiveThemeTokens().accent;

  painter.save();
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setBrush(Qt::NoBrush);

  painter.setPen(QPen{accent, kSelectionInnerWidth});
  painter.drawRect(inner);

  QPen outer{accent, kSelectionOuterWidth, Qt::DashLine};
  outer.setDashPattern({4, 3});
  painter.setPen(outer);
  painter.setOpacity(kSelectionOuterOpacity);
  painter.drawRect(inner.adjusted(-kSelectionOuterMargin,
                                  -kSelectionOuterMargin, kSelectionOuterMargin,
                                  kSelectionOuterMargin));

  painter.restore();
}

void DisplayWidget::SetSelection(std::optional<ShapeHit> selection) {
  const bool had_selection = selection_.has_value();
  if (!had_selection && !selection)
    return;
  if (had_selection && selection && selection_->id == selection->id)
    return;

  selection_ = std::move(selection);
  update();

  if (selection_callback_)
    selection_callback_(selection_);
}

QPointF DisplayWidget::WidgetToPage(const QPoint& point) const {
  if (!document_ || width() == 0 || height() == 0)
    return {};

  const RectF bounds = document_->PageBounds();
  if (!std::isfinite(bounds.w) || !std::isfinite(bounds.h) || bounds.w <= 0 ||
      bounds.h <= 0) {
    return {};
  }

  // Page Y grows upward, widget Y downward.
  const double scale_x = bounds.w / width();
  const double scale_y = bounds.h / height();
  return {point.x() * scale_x, bounds.h - point.y() * scale_y};
}
