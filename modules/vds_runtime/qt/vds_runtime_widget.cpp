#include "vds_runtime/qt/vds_runtime_widget.h"

#include <QMouseEvent>
#include <QPainter>

namespace {

std::string ToUtf8(const std::filesystem::path& path) {
  return QString::fromStdWString(path.wstring()).toUtf8().toStdString();
}

QString FromUtf8(const char* text) {
  return QString::fromUtf8(text ? text : "");
}

}  // namespace

VdsRuntimeWidget::VdsRuntimeWidget(QWidget* parent) : QWidget{parent} {
  setMouseTracking(true);
  setFocusPolicy(Qt::ClickFocus);
}

VdsRuntimeWidget::~VdsRuntimeWidget() {
  if (document_)
    loader_.api().close_document(document_);
}

bool VdsRuntimeWidget::Open(const std::filesystem::path& path, int32_t kind) {
  path_ = path;
  error_message_.clear();

  if (document_) {
    loader_.api().close_document(document_);
    document_ = nullptr;
  }

  if (!loader_.is_loaded()) {
    error_message_ = loader_.error_message();
    update();
    return false;
  }

  TcVdsRuntimeError error{};
  const auto utf8_path = ToUtf8(path);
  document_ = loader_.api().open_document(utf8_path.c_str(), kind, &error);
  if (!document_) {
    error_message_ = FormatError(QStringLiteral("Cannot open document"), error);
    update();
    return false;
  }

  if (!loader_.api().get_document_info(document_, &document_info_, &error)) {
    error_message_ =
        FormatError(QStringLiteral("Cannot read document info"), error);
    update();
    return false;
  }

  title_ = FromUtf8(document_info_.title);
  updateGeometry();
  update();
  return true;
}

QSize VdsRuntimeWidget::sizeHint() const {
  if (!document_)
    return {640, 480};

  return {static_cast<int>(document_info_.bounds.width),
          static_cast<int>(document_info_.bounds.height)};
}

void VdsRuntimeWidget::paintEvent(QPaintEvent*) {
  QPainter painter{this};

  if (!document_) {
    painter.fillRect(rect(), QColor{255, 255, 255});
    painter.setPen(QColor{160, 0, 0});
    painter.drawText(rect().adjusted(24, 24, -24, -24),
                     Qt::AlignCenter | Qt::TextWordWrap,
                     error_message_.isEmpty()
                         ? QStringLiteral("VDS runtime is not available.")
                         : error_message_);
    return;
  }

  QImage image{size(), QImage::Format_ARGB32};
  TcVdsRuntimeError error{};
  if (!loader_.api().render_bgra(document_, image.bits(), image.width(),
                                 image.height(), image.bytesPerLine(),
                                 &error)) {
    painter.fillRect(rect(), QColor{255, 255, 255});
    painter.setPen(QColor{160, 0, 0});
    painter.drawText(
        rect().adjusted(24, 24, -24, -24), Qt::AlignCenter | Qt::TextWordWrap,
        FormatError(QStringLiteral("Cannot render document"), error));
    return;
  }

  painter.drawImage(0, 0, image);
}

void VdsRuntimeWidget::mousePressEvent(QMouseEvent* event) {
  setFocus(Qt::MouseFocusReason);
  if (!document_ || !selection_callback_)
    return;

  auto page = WidgetToPage(event->pos());
  TcVdsRuntimeShapeInfo shape{};
  TcVdsRuntimeError error{};
  if (loader_.api().hit_test(document_, page.x(), page.y(), &shape, &error)) {
    selection_callback_(FromUtf8(shape.data_source));
    event->accept();
  }
}

void VdsRuntimeWidget::mouseDoubleClickEvent(QMouseEvent* event) {
  if (double_click_callback_) {
    double_click_callback_();
    event->accept();
  }
}

QString VdsRuntimeWidget::FormatError(const QString& title,
                                      const TcVdsRuntimeError& error) const {
  return QStringLiteral("%1: %2").arg(title, FromUtf8(error.message));
}

QPointF VdsRuntimeWidget::WidgetToPage(const QPoint& point) const {
  if (!document_ || width() == 0 || height() == 0)
    return {};

  const double scale_x = document_info_.bounds.width / width();
  const double scale_y = document_info_.bounds.height / height();
  return {point.x() * scale_x,
          document_info_.bounds.height - point.y() * scale_y};
}
