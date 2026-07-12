#include "main_window/activity_bar_qt.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"

#include <QButtonGroup>
#include <QFont>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QPolygonF>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

constexpr int kRailWidth = 52;
constexpr int kButtonSize = 44;
constexpr int kIconSize = 24;

// The design-token set for the active theme. The rail is only built when a
// token theme is active (see MainWindow), so mapping the legacy case to dark is
// a harmless fallback.
const scada::aui::ThemeTokens& RailTokens() {
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

// Draws the dedicated line glyph for a section within `r` (a square icon box)
// using the already-configured pen. Simple 2px vector marks, so they stay crisp
// on the charcoal rail at any DPI without shipping raster assets.
void DrawSectionGlyph(QPainter& p, ActivityBar::Icon kind, const QRectF& r) {
  const qreal x = r.x(), y = r.y(), w = r.width(), h = r.height();
  switch (kind) {
    case ActivityBar::Icon::kOverview: {
      // 2x2 dashboard grid.
      const qreal g = w * 0.12, cell = (w - g) / 2 - 1;
      for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
          p.drawRoundedRect(
              QRectF(x + i * (cell + g), y + j * (cell + g), cell, cell), 1.5,
              1.5);
      break;
    }
    case ActivityBar::Icon::kAlarms: {
      // Bell: rounded body + base line + clapper.
      QPainterPath bell;
      bell.moveTo(x + w * 0.2, y + h * 0.68);
      bell.cubicTo(x + w * 0.2, y + h * 0.25, x + w * 0.8, y + h * 0.25,
                   x + w * 0.8, y + h * 0.68);
      p.drawPath(bell);
      p.drawLine(QPointF(x + w * 0.12, y + h * 0.68),
                 QPointF(x + w * 0.88, y + h * 0.68));
      p.drawLine(QPointF(x + w * 0.42, y + h * 0.82),
                 QPointF(x + w * 0.58, y + h * 0.82));
      break;
    }
    case ActivityBar::Icon::kTrends: {
      // Rising trend polyline with a couple of vertices.
      QPolygonF line;
      line << QPointF(x + w * 0.12, y + h * 0.72)
           << QPointF(x + w * 0.38, y + h * 0.5)
           << QPointF(x + w * 0.58, y + h * 0.62)
           << QPointF(x + w * 0.88, y + h * 0.28);
      p.drawPolyline(line);
      break;
    }
    case ActivityBar::Icon::kSubstations: {
      // Schematic: top busbar feeding a node (transformer-ish circle).
      p.drawLine(QPointF(x + w * 0.15, y + h * 0.22),
                 QPointF(x + w * 0.85, y + h * 0.22));
      p.drawLine(QPointF(x + w * 0.5, y + h * 0.22),
                 QPointF(x + w * 0.5, y + h * 0.45));
      p.drawEllipse(QRectF(x + w * 0.32, y + h * 0.45, w * 0.36, h * 0.36));
      break;
    }
    case ActivityBar::Icon::kTables: {
      // Table: outer rect + two row dividers.
      p.drawRoundedRect(QRectF(x + w * 0.14, y + h * 0.2, w * 0.72, h * 0.6),
                        1.5, 1.5);
      p.drawLine(QPointF(x + w * 0.14, y + h * 0.4),
                 QPointF(x + w * 0.86, y + h * 0.4));
      p.drawLine(QPointF(x + w * 0.14, y + h * 0.6),
                 QPointF(x + w * 0.86, y + h * 0.6));
      break;
    }
    case ActivityBar::Icon::kAdministration: {
      // Person: head + shoulders.
      p.drawEllipse(QRectF(x + w * 0.36, y + h * 0.18, w * 0.28, h * 0.28));
      QPainterPath body;
      body.moveTo(x + w * 0.2, y + h * 0.82);
      body.cubicTo(x + w * 0.2, y + h * 0.52, x + w * 0.8, y + h * 0.52,
                   x + w * 0.8, y + h * 0.82);
      p.drawPath(body);
      break;
    }
    case ActivityBar::Icon::kSettings: {
      // Three setting sliders with offset knobs.
      const qreal ys[] = {y + h * 0.28, y + h * 0.5, y + h * 0.72};
      const qreal knobs[] = {x + w * 0.66, x + w * 0.34, x + w * 0.58};
      for (int i = 0; i < 3; ++i) {
        p.drawLine(QPointF(x + w * 0.16, ys[i]), QPointF(x + w * 0.84, ys[i]));
        p.drawEllipse(QPointF(knobs[i], ys[i]), w * 0.07, w * 0.07);
      }
      break;
    }
    case ActivityBar::Icon::kNone:
      break;
  }
}

// A rail button icon that always shows something: the dedicated section glyph,
// or a charcoal-friendly glyph of the label's first letter when the section has
// no dedicated icon.
QIcon SectionIcon(const ActivityBar::Section& section, const QColor& fg) {
  QPixmap pixmap{kIconSize, kIconSize};
  pixmap.fill(Qt::transparent);
  QPainter painter{&pixmap};
  painter.setRenderHint(QPainter::Antialiasing);

  if (section.icon_kind != ActivityBar::Icon::kNone) {
    QPen pen{fg};
    pen.setWidthF(1.8);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    // Inset so the 2px stroke stays inside the box.
    DrawSectionGlyph(painter, section.icon_kind,
                     QRectF(2, 2, kIconSize - 4, kIconSize - 4));
    return QIcon{pixmap};
  }

  painter.setPen(fg);
  QFont font = painter.font();
  font.setPixelSize(kIconSize - 6);
  font.setBold(true);
  painter.setFont(font);
  const QString glyph =
      section.label.empty()
          ? QStringLiteral("?")
          : QString::fromStdU16String(section.label.substr(0, 1)).toUpper();
  painter.drawText(pixmap.rect(), Qt::AlignCenter, glyph);
  return QIcon{pixmap};
}

}  // namespace

ActivityBar::ActivityBar(QWidget* parent,
                         std::vector<Section> sections,
                         ActivateCallback on_activate)
    : QWidget{parent}, on_activate_{std::move(on_activate)} {
  setObjectName(QStringLiteral("activityBar"));
  setFixedWidth(kRailWidth);

  const scada::aui::ThemeTokens& tokens = RailTokens();
  // Charcoal rail with token-driven active/hover states; the checked (active)
  // section gets an accent left-marker and a soft accent fill.
  setStyleSheet(
      QStringLiteral(
          "#activityBar { background: %1; }"
          "#activityBar QToolButton { border: none; border-left: 3px solid "
          "transparent; background: transparent; }"
          "#activityBar QToolButton:hover { background: %2; }"
          "#activityBar QToolButton:checked { border-left: 3px solid %3; "
          "background: %4; }"
          "#activityBar QToolButton:disabled { }")
          .arg(tokens.rail_bg.name(), tokens.surface_muted.name(),
               tokens.accent.name(), tokens.accent_soft.name()));

  group_ = new QButtonGroup{this};
  group_->setExclusive(true);

  auto* layout = new QVBoxLayout{this};
  layout->setContentsMargins(0, 6, 0, 6);
  layout->setSpacing(2);

  auto add_button = [&](const Section& section) {
    auto* button = new QToolButton{this};
    button->setCheckable(true);
    button->setAutoRaise(true);
    button->setFixedSize(kButtonSize, kButtonSize);
    button->setIconSize({kIconSize, kIconSize});
    button->setIcon(SectionIcon(section, tokens.fg_on_dark));
    button->setToolTip(QString::fromStdU16String(section.label));
    button->setEnabled(section.enabled);
    if (!section.enabled) {
      button->setToolTip(QString::fromStdU16String(section.label) +
                         QStringLiteral(" (coming soon)"));
    }
    group_->addButton(button);
    layout->addWidget(button, 0, Qt::AlignHCenter);

    const std::string name = section.window_info_name;
    Item& item = items_.emplace_back(Item{section, button});
    if (section.is_alarms)
      RefreshAlarmsButton();
    connect(button, &QToolButton::clicked, this, [this, name] {
      if (on_activate_ && !name.empty())
        on_activate_(name);
    });
    return &item;
  };

  for (const Section& section : sections) {
    if (!section.pinned_bottom)
      add_button(section);
  }
  layout->addStretch(1);
  for (const Section& section : sections) {
    if (section.pinned_bottom)
      add_button(section);
  }
}

ActivityBar::~ActivityBar() = default;

void ActivityBar::SetAlarmCount(int count) {
  if (count == alarm_count_)
    return;
  alarm_count_ = count;
  RefreshAlarmsButton();
}

void ActivityBar::SetActiveSection(const std::string& window_info_name) {
  for (const Item& item : items_) {
    if (item.section.window_info_name == window_info_name) {
      item.button->setChecked(true);
      return;
    }
  }
}

void ActivityBar::RefreshAlarmsButton() {
  const scada::aui::ThemeTokens& tokens = RailTokens();
  for (const Item& item : items_) {
    if (!item.section.is_alarms)
      continue;

    QIcon base_icon = SectionIcon(item.section, tokens.fg_on_dark);
    if (alarm_count_ <= 0) {
      item.button->setIcon(base_icon);
      return;
    }

    // Composite a severity-coloured count badge over the top-right of the icon.
    QPixmap pixmap = base_icon.pixmap(kIconSize, kIconSize);
    QPainter painter{&pixmap};
    painter.setRenderHint(QPainter::Antialiasing);
    const int diameter = 14;
    const QRect badge{kIconSize - diameter, 0, diameter, diameter};
    painter.setPen(Qt::NoPen);
    painter.setBrush(tokens.severity_critical);
    painter.drawEllipse(badge);

    const QString text = alarm_count_ > 99 ? QStringLiteral("99+")
                                           : QString::number(alarm_count_);
    QFont font = painter.font();
    font.setPixelSize(alarm_count_ > 99 ? 6 : 9);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(tokens.accent_fg);
    painter.drawText(badge, Qt::AlignCenter, text);
    painter.end();

    item.button->setIcon(QIcon{pixmap});
    return;
  }
}
