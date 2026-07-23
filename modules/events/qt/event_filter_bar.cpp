#include "events/qt/event_filter_bar.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "model/data_items_node_ids.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "scada/standard_node_ids.h"

#include <QCheckBox>
#include <QComboBox>
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

// Labels for the period presets, one per entry in `EventPeriodRanges()`. Kept
// in lock-step with that table. Reuses the existing period-action translations.
std::vector<std::u16string> PeriodLabels() {
  return {Translate("15 min"), Translate("Hour"), Translate("Day"),
          Translate("Week"), Translate("Month")};
}

}  // namespace

Awaitable<std::vector<EventAreaEntry>> BrowseEventAreas(
    NodeService& node_service) {
  std::vector<EventAreaEntry> areas;

  // Browse the same root the object tree uses (`DataItems`, "Все объекты"): its
  // immediate `Organizes` children are the operator-facing top-level groupings.
  // ObjectsFolder is one level too high — it holds the standard OPC folders and
  // the "Все объекты"/"Все оборудование" containers, not the areas themselves.
  co_await node_service.Fetch(scada::data_items::id::DataItems,
                              NodeFetchStatus::NodeAndChildren);

  for (NodeRef& child : node_service.GetTargets(
           scada::data_items::id::DataItems, scada::id::Organizes,
           /*forward=*/true)) {
    co_await child.Fetch(NodeFetchStatus::NodeOnly);
    // Areas are the object groupings above the data items; a top-level leaf
    // data item (e.g. a loose tag) is not an area, so skip it.
    if (IsInstanceOf(child, scada::data_items::id::DataItemType))
      continue;
    areas.push_back({child.node_id(), GetFullDisplayName(child)});
  }

  co_return areas;
}

const std::vector<TimeRange>& EventPeriodRanges() {
  // Mirrors the toolbar's ID_TIME_RANGE_* quick-picks so a range set there
  // reflects onto the matching preset here.
  static const std::vector<TimeRange> ranges = {
      TimeRange{std::chrono::minutes(15)},
      TimeRange{std::chrono::hours(1)},
      TimeRange{TimeRange::Type::Day},
      TimeRange{TimeRange::Type::Week},
      TimeRange{TimeRange::Type::Month},
  };
  return ranges;
}

int EventPeriodPresetIndex(const TimeRange& range) {
  const auto& ranges = EventPeriodRanges();
  for (size_t i = 0; i < ranges.size(); ++i) {
    if (ranges[i] == range)
      return static_cast<int>(i);
  }
  // No quick-pick matches this range (e.g. an arbitrary custom window).
  return -1;
}

QWidget* MakeEventFilterBar(EventFilterBarContext context) {
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
  unacknowledged->setObjectName(QStringLiteral("unacknowledgedOnly"));
  unacknowledged->setChecked(context.unacknowledged_only);
  QObject::connect(unacknowledged, &QCheckBox::toggled, bar,
                   [callback = context.on_unacknowledged_only](bool checked) {
                     if (callback)
                       callback(checked);
                   });
  layout->addWidget(unacknowledged);

  layout->addWidget(
      new QLabel{QString::fromStdU16String(Translate("Min. severity")), bar});
  auto* severity = new QSpinBox{bar};
  severity->setRange(0, static_cast<int>(context.severity_max));
  severity->setValue(static_cast<int>(context.severity_min));
  // 0 reads as "All" — no minimum-severity filter.
  severity->setSpecialValueText(QString::fromStdU16String(Translate("All")));
  QObject::connect(severity, QOverload<int>::of(&QSpinBox::valueChanged), bar,
                   [callback = context.on_severity_min](int value) {
                     if (callback)
                       callback(static_cast<unsigned>(value));
                   });
  layout->addWidget(severity);

  // Period selector: the fixed quick-pick ranges. Reflects the current range
  // when it matches a preset, otherwise stays unselected.
  layout->addWidget(
      new QLabel{QString::fromStdU16String(Translate("Period")), bar});
  auto* period = new QComboBox{bar};
  for (const std::u16string& label : PeriodLabels())
    period->addItem(QString::fromStdU16String(label));
  period->setCurrentIndex(EventPeriodPresetIndex(context.time_range));
  QObject::connect(
      period, QOverload<int>::of(&QComboBox::activated), bar,
      [callback = context.on_time_range](int index) {
        const auto& ranges = EventPeriodRanges();
        if (index < 0 || static_cast<size_t>(index) >= ranges.size())
          return;
        if (callback)
          callback(ranges[index]);
      });
  layout->addWidget(period);

  layout->addStretch(1);
  return bar;
}
