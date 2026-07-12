#include "display_frame/qt/display_frame.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "vds_runtime/qt/vds_runtime_widget.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace {

constexpr double kMinZoom = 0.05;
constexpr double kMaxZoom = 8.0;
constexpr double kZoomStep = 1.25;

// The design tokens for the active reshell theme. The frame is only built under
// a token theme (WrapDisplayInFrame gates on it), so the legacy fallback here
// is harmless. Mirrors the BarTokens() helper in the event filter bar.
const scada::aui::ThemeTokens& FrameTokens() {
  scada::aui::Theme theme = scada::aui::Theme::kDark;
  switch (scada::aui::GetSeverityTheme()) {
    case scada::aui::SeverityTheme::kLight:
      theme = scada::aui::Theme::kLight;
      break;
    case scada::aui::SeverityTheme::kHighContrast:
      theme = scada::aui::Theme::kHighContrast;
      break;
    default:
      break;
  }
  return scada::aui::GetThemeTokens(theme);
}

// A translucent "soft" tint of `color` for pill / chip fills, matching the
// mockup's `--good-soft` style (the token set exposes only the solid colours).
QString SoftRgba(const QColor& color, double alpha) {
  return QStringLiteral("rgba(%1,%2,%3,%4)")
      .arg(color.red())
      .arg(color.green())
      .arg(color.blue())
      .arg(alpha, 0, 'f', 2);
}

QString Tr(std::string_view text) {
  return QString::fromStdU16String(Translate(text));
}

}  // namespace

double ClampDisplayZoom(double zoom) {
  if (!std::isfinite(zoom))
    return 1.0;
  return std::clamp(zoom, kMinZoom, kMaxZoom);
}

double DisplayFitFactor(QSize natural, QSize viewport) {
  if (natural.width() <= 0 || natural.height() <= 0 || viewport.width() <= 0 ||
      viewport.height() <= 0) {
    return 1.0;
  }
  const double fx = static_cast<double>(viewport.width()) / natural.width();
  const double fy = static_cast<double>(viewport.height()) / natural.height();
  return ClampDisplayZoom(std::min(fx, fy));
}

int DisplayZoomPercent(double zoom) {
  return static_cast<int>(std::lround(ClampDisplayZoom(zoom) * 100.0));
}

DisplayFrame::DisplayFrame(VdsRuntimeWidget* diagram,
                           QString breadcrumb,
                           QWidget* parent)
    : QWidget{parent}, diagram_{diagram} {
  auto* root = new QVBoxLayout{this};
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(0);

  BuildToolbar(breadcrumb);

  scroll_ = new QScrollArea{this};
  scroll_->setWidgetResizable(false);
  scroll_->setAlignment(Qt::AlignCenter);
  scroll_->setFrameShape(QFrame::NoFrame);
  if (diagram_)
    scroll_->setWidget(diagram_);
  root->addWidget(scroll_, /*stretch=*/1);

  const scada::aui::ThemeTokens& tokens = FrameTokens();
  scroll_->viewport()->setStyleSheet(
      QStringLiteral("background:%1;").arg(tokens.bg.name()));

  // Refit when the viewport is (re)sized — it gets its real size only after the
  // frame's layout runs, which is after the frame's own resizeEvent.
  scroll_->viewport()->installEventFilter(this);

  ApplyZoom();
}

DisplayFrame::~DisplayFrame() = default;

void DisplayFrame::BuildToolbar(const QString& breadcrumb) {
  const scada::aui::ThemeTokens& tokens = FrameTokens();

  auto* bar = new QWidget{this};
  bar->setObjectName(QStringLiteral("displayToolbar"));
  bar->setStyleSheet(
      QStringLiteral(
          "#displayToolbar{background:%1;border-bottom:1px solid %2;}"
          "#displayToolbar QToolButton{color:%3;border:none;padding:2px 8px;}"
          "#displayToolbar QToolButton:hover{color:%4;}"
          "#displayToolbar QLabel{color:%3;}")
          .arg(tokens.bg_elevated.name(), tokens.border.name(),
               tokens.fg_muted.name(), tokens.fg.name()));

  auto* layout = new QHBoxLayout{bar};
  layout->setContentsMargins(12, 4, 12, 4);
  layout->setSpacing(10);

  // Hotspot breadcrumb — the display's location / title.
  if (!breadcrumb.isEmpty()) {
    auto* crumb = new QLabel{breadcrumb, bar};
    crumb->setStyleSheet(
        QStringLiteral("color:%1;font-weight:600;").arg(tokens.fg.name()));
    layout->addWidget(crumb);
  }

  // Live indicator pill.
  auto* live = new QLabel{Tr("Live"), bar};
  live->setObjectName(QStringLiteral("liveIndicator"));
  live->setStyleSheet(
      QStringLiteral("#liveIndicator{background:%1;color:%2;border-radius:9px;"
                     "padding:1px 10px;font-weight:600;}")
          .arg(SoftRgba(tokens.good, 0.16), tokens.good.name()));
  layout->addWidget(live);

  layout->addStretch(1);

  // Zoom / fit / 100% controls.
  auto* zoom_out = new QToolButton{bar};
  zoom_out->setText(QStringLiteral("−"));  // minus sign
  zoom_out->setToolTip(Tr("Zoom out"));
  connect(zoom_out, &QToolButton::clicked, this,
          [this] { SetZoom(zoom_ / kZoomStep); });
  layout->addWidget(zoom_out);

  zoom_label_ = new QLabel{QStringLiteral("100%"), bar};
  zoom_label_->setMinimumWidth(40);
  zoom_label_->setAlignment(Qt::AlignCenter);
  layout->addWidget(zoom_label_);

  auto* zoom_in = new QToolButton{bar};
  zoom_in->setText(QStringLiteral("+"));
  zoom_in->setToolTip(Tr("Zoom in"));
  connect(zoom_in, &QToolButton::clicked, this,
          [this] { SetZoom(zoom_ * kZoomStep); });
  layout->addWidget(zoom_in);

  auto* fit = new QToolButton{bar};
  fit->setText(Tr("Fit"));
  fit->setToolTip(Tr("Fit to window"));
  connect(fit, &QToolButton::clicked, this, [this] {
    fit_ = true;
    RefitToViewport();
  });
  layout->addWidget(fit);

  auto* reset = new QToolButton{bar};
  reset->setText(QStringLiteral("100%"));
  reset->setToolTip(Tr("Actual size"));
  connect(reset, &QToolButton::clicked, this, [this] { SetZoom(1.0); });
  layout->addWidget(reset);

  auto* export_button = new QToolButton{bar};
  export_button->setText(Tr("Export"));
  export_button->setToolTip(Tr("Export image"));
  connect(export_button, &QToolButton::clicked, this,
          [this] { ExportImage(); });
  layout->addWidget(export_button);

  qobject_cast<QVBoxLayout*>(this->layout())->insertWidget(0, bar);
}

QSize DisplayFrame::DiagramNaturalSize() const {
  if (!diagram_)
    return {640, 480};
  QSize hint = diagram_->sizeHint();
  return {std::max(hint.width(), 1), std::max(hint.height(), 1)};
}

void DisplayFrame::ApplyZoom() {
  if (!diagram_)
    return;
  const QSize natural = DiagramNaturalSize();
  diagram_->setFixedSize(
      QSize{static_cast<int>(std::lround(natural.width() * zoom_)),
            static_cast<int>(std::lround(natural.height() * zoom_))});
  if (zoom_label_)
    zoom_label_->setText(QStringLiteral("%1%").arg(DisplayZoomPercent(zoom_)));
}

void DisplayFrame::RefitToViewport() {
  if (!scroll_)
    return;
  zoom_ = DisplayFitFactor(DiagramNaturalSize(), scroll_->viewport()->size());
  ApplyZoom();
}

void DisplayFrame::SetZoom(double zoom) {
  fit_ = false;
  zoom_ = ClampDisplayZoom(zoom);
  ApplyZoom();
}

bool DisplayFrame::eventFilter(QObject* watched, QEvent* event) {
  if (scroll_ && watched == scroll_->viewport() &&
      event->type() == QEvent::Resize && fit_) {
    RefitToViewport();
  }
  return QWidget::eventFilter(watched, event);
}

void DisplayFrame::ExportImage() {
  if (!diagram_)
    return;
  const QString path = QFileDialog::getSaveFileName(
      this, Tr("Export image"), QString{}, QStringLiteral("PNG (*.png)"));
  if (path.isEmpty())
    return;
  diagram_->grab().save(path);
}

QWidget* WrapDisplayInFrame(VdsRuntimeWidget* diagram, QString breadcrumb) {
  if (scada::aui::GetSeverityTheme() == scada::aui::SeverityTheme::kLegacy)
    return diagram;
  return new DisplayFrame{diagram, std::move(breadcrumb)};
}
