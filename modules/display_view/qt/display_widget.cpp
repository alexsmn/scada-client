#include "display_view/qt/display_widget.h"

#include "aui/translation.h"

#include <QMouseEvent>
#include <QPainter>

#include <algorithm>
#include <cmath>

namespace {

using scada::display::view::DisplayDocument;
using scada::display::view::DocumentKind;
using scada::display::view::Error;
using scada::display::RectF;

QString Tr(std::string_view text) {
  return QString::fromStdU16String(Translate(text));
}

int SizeHintDimension(double value, int fallback) {
  if (!std::isfinite(value) || value <= 0)
    return fallback;
  return static_cast<int>(std::clamp(std::ceil(value), 1.0, 8192.0));
}

}  // namespace

DisplayWidget::DisplayWidget(QWidget* parent) : QWidget{parent} {
  setMouseTracking(true);
  setFocusPolicy(Qt::ClickFocus);
}

DisplayWidget::~DisplayWidget() = default;

bool DisplayWidget::Open(const std::filesystem::path& path, DocumentKind kind) {
  path_ = path;
  title_.clear();
  error_message_.clear();
  document_.reset();

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
  }
}

void DisplayWidget::mousePressEvent(QMouseEvent* event) {
  setFocus(Qt::MouseFocusReason);
  if (!document_ || !selection_callback_)
    return;

  const QPointF page = WidgetToPage(event->pos());
  if (!std::isfinite(page.x()) || !std::isfinite(page.y()))
    return;

  const std::optional<scada::display::view::ShapeHit> hit =
      document_->HitTest({page.x(), page.y()});
  if (hit) {
    selection_callback_(QString::fromStdString(hit->data_source));
    event->accept();
  }
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
