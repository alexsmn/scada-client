#include "events/qt/event_filter_bar.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QWidget>

#include <utility>

namespace {

// The design tokens for the active theme (the bar is only built under a token
// theme, so the legacy fallback is harmless).
const scada::aui::ThemeTokens& BarTokens() {
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

}  // namespace

QWidget* MakeEventFilterBar(bool unacknowledged_only,
                            unsigned severity_min,
                            unsigned severity_max,
                            std::function<void(bool)> on_unacknowledged_only,
                            std::function<void(unsigned)> on_severity_min) {
  auto* bar = new QWidget;
  bar->setObjectName(QStringLiteral("eventFilterBar"));

  const scada::aui::ThemeTokens& tokens = BarTokens();
  bar->setStyleSheet(
      QStringLiteral(
          "#eventFilterBar{background:%1;border-bottom:1px solid %2;}")
          .arg(tokens.bg_elevated.name(), tokens.border.name()));

  auto* layout = new QHBoxLayout{bar};
  layout->setContentsMargins(10, 5, 10, 5);
  layout->setSpacing(12);

  auto* unacknowledged = new QCheckBox{
      QString::fromStdU16String(Translate("Unacknowledged only")), bar};
  unacknowledged->setChecked(unacknowledged_only);
  QObject::connect(
      unacknowledged, &QCheckBox::toggled, bar,
      [callback = std::move(on_unacknowledged_only)](bool checked) {
        if (callback)
          callback(checked);
      });
  layout->addWidget(unacknowledged);

  layout->addWidget(
      new QLabel{QString::fromStdU16String(Translate("Min. severity")), bar});
  auto* severity = new QSpinBox{bar};
  severity->setRange(0, static_cast<int>(severity_max));
  severity->setValue(static_cast<int>(severity_min));
  // 0 reads as "All" — no minimum-severity filter.
  severity->setSpecialValueText(QString::fromStdU16String(Translate("All")));
  QObject::connect(severity, QOverload<int>::of(&QSpinBox::valueChanged), bar,
                   [callback = std::move(on_severity_min)](int value) {
                     if (callback)
                       callback(static_cast<unsigned>(value));
                   });
  layout->addWidget(severity);

  layout->addStretch(1);
  return bar;
}
