#include "events/qt/event_filter_bar.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "scada/standard_node_ids.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
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

// A top-level area: a container node directly under ObjectsFolder that events
// can be filtered by (a data item is "in" the area when the area is one of its
// containing nodes, which `EventTableModel::IsEventShown` already honours).
struct AreaEntry {
  scada::NodeId node_id;
  std::u16string name;
};

// Browses the immediate children of ObjectsFolder (the top-level areas) and
// returns them as {node_id, display name}. `node_service` is captured by
// reference and outlives the browse; the caller guards its own widgets.
Awaitable<std::vector<AreaEntry>> BrowseAreasAsync(NodeService& node_service) {
  std::vector<AreaEntry> areas;

  co_await node_service.Fetch(scada::id::ObjectsFolder,
                              NodeFetchStatus::NodeAndChildren);

  for (NodeRef& child : node_service.GetTargets(
           scada::id::ObjectsFolder, scada::id::Organizes, /*forward=*/true)) {
    co_await child.Fetch(NodeFetchStatus::NodeOnly);
    areas.push_back({child.node_id(), GetFullDisplayName(child)});
  }

  co_return areas;
}

// Labels for the period presets, one per entry in `EventPeriodRanges()`. Kept
// in lock-step with that table. Reuses the existing period-action translations.
std::vector<std::u16string> PeriodLabels() {
  return {Translate("15 min"), Translate("Hour"), Translate("Day"),
          Translate("Week"), Translate("Month")};
}

}  // namespace

const std::vector<TimeRange>& EventPeriodRanges() {
  // Mirrors the toolbar's ID_TIME_RANGE_* quick-picks so a range set there
  // reflects onto the matching preset here.
  static const std::vector<TimeRange> ranges = {
      TimeRange{base::TimeDelta::FromMinutes(15)},
      TimeRange{base::TimeDelta::FromHours(1)},
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

  // Area selector: "All areas" plus the top-level areas, filled in
  // asynchronously once the address-space browse completes.
  layout->addWidget(
      new QLabel{QString::fromStdU16String(Translate("Area")), bar});
  auto* area = new QComboBox{bar};
  area->addItem(QString::fromStdU16String(Translate("All areas")));
  // Area ids, aligned with the combo entries at index+1 (index 0 is "All").
  auto area_ids = std::make_shared<std::vector<scada::NodeId>>();
  QObject::connect(area, QOverload<int>::of(&QComboBox::activated), bar,
                   [area_ids, callback = context.on_area](int index) {
                     if (!callback)
                       return;
                     if (index <= 0)
                       callback(std::nullopt);
                     else if (static_cast<size_t>(index - 1) < area_ids->size())
                       callback((*area_ids)[index - 1]);
                   });
  CoSpawn(context.executor,
          [&node_service = context.node_service, area_ids,
           combo = QPointer<QComboBox>{area}]() -> Awaitable<void> {
            auto areas = co_await BrowseAreasAsync(node_service);
            if (!combo)
              co_return;
            for (const AreaEntry& entry : areas) {
              area_ids->push_back(entry.node_id);
              combo->addItem(QString::fromStdU16String(entry.name));
            }
          });
  layout->addWidget(area);

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
