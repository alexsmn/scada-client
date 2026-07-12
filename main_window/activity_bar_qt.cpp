#include "main_window/activity_bar_qt.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"

#include <QButtonGroup>
#include <QFont>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
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

// A rail button icon that always shows something: the resolved command icon, or
// a charcoal-friendly glyph of the label's first letter when the view has none.
QIcon SectionIcon(const ActivityBar::Section& section, const QColor& fg) {
  if (!section.icon.isNull())
    return section.icon;

  QPixmap pixmap{kIconSize, kIconSize};
  pixmap.fill(Qt::transparent);
  QPainter painter{&pixmap};
  painter.setRenderHint(QPainter::Antialiasing);
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
