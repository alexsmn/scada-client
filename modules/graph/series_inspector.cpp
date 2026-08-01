#include "graph/series_inspector.h"

#if defined(UI_QT)
#include "graph/limit_markers.h"
#include "graph/metrix_data_source.h"

#include "aui/color.h"
#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"

#include "scada/qualifier.h"

#include <QFont>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>

#include <optional>

namespace {

// Panel geometry (device-independent pixels), tracking the reshell mockup
// docs/product/ui-mockups/screens/trend.html (.insp / .insec / .row / .swatch).
constexpr int kWidth = 290;
constexpr int kPadX = 14;
constexpr int kHeaderH = 44;  // title bar height
constexpr int kSectionGap = 14;
constexpr int kSecHeadH = 20;  // uppercase section label row
constexpr int kRowH = 26;
constexpr int kSwatch = 22;  // colour swatch size
constexpr int kSwatchGap = 6;
constexpr int kToggleW = 34;
constexpr int kToggleH = 18;
constexpr int kHeaderSwatchW = 12;
constexpr int kHeaderSwatchH = 3;

QFont LabelFont(int pixel_size, bool bold = false) {
  QFont font;
  font.setPixelSize(pixel_size);
  font.setBold(bold);
  return font;
}

}  // namespace

SeriesInspector::SeriesInspector(QWidget* parent) : QWidget(parent) {}

QSize SeriesInspector::sizeHint() const {
  return QSize(kWidth, 480);
}

void SeriesInspector::SetLine(MetrixGraph::MetrixLine* line) {
  line_ = line;
  update();
}

void SeriesInspector::mousePressEvent(QMouseEvent* event) {
  for (const SwatchHit& hit : swatch_hits_) {
    if (hit.rect.contains(event->pos())) {
      if (on_color_chosen)
        on_color_chosen(hit.color);
      return;
    }
  }
  QWidget::mousePressEvent(event);
}

void SeriesInspector::paintEvent(QPaintEvent*) {
  swatch_hits_.clear();

  if (!line_)
    return;
  // Tokens come from ActiveThemeTokens(), which maps the legacy theme onto the
  // dark table on purpose: the panel must paint whoever built it. Gating the
  // paint on the reshell theme instead left the standalone offscreen capture
  // (series-inspector.png) blank in every un-themed generator run, because the
  // capture builds the widget directly rather than through GraphView — which
  // is where the opt-in gate belongs, and already lives.
  const scada::aui::ThemeTokens& tokens = scada::aui::ActiveThemeTokens();
  const MetrixDataSource& source = line_->data_source();

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.fillRect(rect(), tokens.bg_elevated);

  // Header: series colour swatch + title.
  painter.setPen(QPen{tokens.border});
  painter.drawLine(0, kHeaderH, width(), kHeaderH);
  painter.setPen(Qt::NoPen);
  painter.setBrush(line_->color());
  painter.drawRoundedRect(kPadX, kHeaderH / 2 - kHeaderSwatchH / 2,
                          kHeaderSwatchW, kHeaderSwatchH, 1.5, 1.5);
  painter.setPen(tokens.fg);
  painter.setFont(LabelFont(13, true));
  const QString title = QString::fromStdU16String(source.title());
  painter.drawText(
      QRect(kPadX + kHeaderSwatchW + 8, 0, width() - kPadX * 2, kHeaderH),
      Qt::AlignLeft | Qt::AlignVCenter,
      QString::fromStdU16String(Translate("Series")) + u" · " + title);

  int y = kHeaderH + 12;

  // Draws an uppercase section label and advances past it.
  auto section = [&](const char* key) {
    painter.setPen(tokens.fg_subtle);
    painter.setFont(LabelFont(11));
    painter.drawText(QRect(kPadX, y, width() - kPadX * 2, kSecHeadH),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QString::fromStdU16String(Translate(key)).toUpper());
    y += kSecHeadH;
  };

  // Draws a "label ........ value" row; `value_color` invalid => default text.
  auto row = [&](const QString& label, const QString& value,
                 QColor value_color = QColor()) {
    painter.setPen(tokens.fg_muted);
    painter.setFont(LabelFont(12));
    const QRect r(kPadX, y, width() - kPadX * 2, kRowH);
    painter.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, label);
    painter.setPen(value_color.isValid() ? value_color : tokens.fg);
    painter.drawText(r, Qt::AlignRight | Qt::AlignVCenter, value);
    y += kRowH;
  };

  // Draws a read-only on/off pill at the right of a labelled row.
  auto toggle_row = [&](const char* key, bool on) {
    painter.setPen(tokens.fg_muted);
    painter.setFont(LabelFont(12));
    const QRect r(kPadX, y, width() - kPadX * 2, kRowH);
    painter.drawText(r, Qt::AlignLeft | Qt::AlignVCenter,
                     QString::fromStdU16String(Translate(key)));
    const QRect pill(r.right() - kToggleW, r.center().y() - kToggleH / 2,
                     kToggleW, kToggleH);
    painter.setPen(on ? Qt::NoPen : QPen{tokens.border_strong});
    painter.setBrush(on ? tokens.accent : tokens.surface_muted);
    painter.drawRoundedRect(pill, kToggleH / 2.0, kToggleH / 2.0);
    const int knob = kToggleH - 4;
    const int knob_x = on ? pill.right() - knob - 2 : pill.left() + 2;
    painter.setPen(Qt::NoPen);
    painter.setBrush(on ? QColor(Qt::white) : tokens.fg_subtle);
    painter.drawEllipse(knob_x, pill.top() + 2, knob, knob);
    y += kRowH;
  };

  // Colour: palette swatches, the active one ringed.
  section("Colour");
  int sx = kPadX;
  const std::size_t color_count = scada::aui::GetColorCount();
  for (std::size_t i = 0; i < color_count; ++i) {
    const QColor color = scada::aui::GetColor(static_cast<int>(i)).qcolor();
    if (sx + kSwatch > width() - kPadX) {
      sx = kPadX;
      y += kSwatch + kSwatchGap;
    }
    const QRect box(sx, y, kSwatch, kSwatch);
    swatch_hits_.push_back({box, color});
    painter.setPen(color == line_->color() ? QPen{tokens.fg, 2} : Qt::NoPen);
    painter.setBrush(color);
    painter.drawRoundedRect(box.adjusted(1, 1, -1, -1), 4, 4);
    sx += kSwatch + kSwatchGap;
  }
  y += kSwatch + kSectionGap;

  // Appearance flags. (Not "Display" — that key already translates to the
  // schematic-display domain term.)
  section("Appearance");
  // "Own pane" is true when the series is alone in its pane.
  toggle_row("Own pane", line_->plot().lines().size() == 1);
  toggle_row("Show dots", line_->dots_shown());
  toggle_row("Stepped", line_->stepped());
  row(QString::fromStdU16String(Translate("Y-axis")),
      QString::fromStdU16String(Translate("Auto")));
  y += kSectionGap;

  // Limits & annotations.
  section("Limits & annotations");
  const std::vector<LimitMarker> markers = ComputeLimitMarkers(
      source.limit_lolo(), source.limit_lo(), source.limit_hi(),
      source.limit_hihi(), kGraphUnknownValue);
  for (const LimitMarker& marker : markers) {
    const std::optional<scada::aui::Color> color =
        scada::aui::SeverityColor(SeverityOf(marker.kind));
    row(QString::fromStdU16String(Translate(LimitBandNameKey(marker.kind))),
        source.GetYAxisLabel(marker.value), color ? color->qcolor() : QColor());
  }
  if (markers.empty())
    row(QString::fromStdU16String(Translate("No limits configured")),
        QString());
  y += kSectionGap;

  // Source.
  section("Source");
  row(QString::fromStdU16String(Translate("Node")),
      QString::fromStdString(source.GetPath()));
  const scada::DataValue current = source.timed_data().current();
  const bool good = current.qualifier.good();
  row(QString::fromStdU16String(Translate("Quality")),
      QString::fromStdU16String(Translate(good ? "Good" : "Bad")),
      good ? tokens.good : tokens.bad);
}
#endif  // UI_QT
