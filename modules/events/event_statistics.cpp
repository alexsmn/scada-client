#include "events/event_statistics.h"

#include "aui/text_fold.h"
#include "aui/translation.h"
#include "base/utf_convert.h"
#include "events/event_severity.h"

#include <algorithm>
#include <map>

namespace events {

namespace {

void Count(EventStatistics::Tally& tally, bool pending) {
  ++tally.count;
  if (pending)
    ++tally.unacknowledged;
}

void AppendTallyRow(std::u16string& text,
                    std::u16string_view title,
                    const EventStatistics::Tally& tally) {
  text += title;
  text += u'\t';
  text += UtfConvert<char16_t>(std::to_string(tally.count));
  text += u'\t';
  text += UtfConvert<char16_t>(std::to_string(tally.unacknowledged));
  text += u'\n';
}

}  // namespace

EventStatistics ComputeEventStatistics(
    std::span<const EventStatisticsSample> samples) {
  EventStatistics statistics;

  // By source rather than by name: two objects may share a display name, and
  // they are still two objects.
  std::map<scada::NodeId, size_t> object_rows;

  for (const EventStatisticsSample& sample : samples) {
    Count(statistics.total, sample.pending);
    Count(statistics.by_band[static_cast<size_t>(
              SeverityLevelForEvent(sample.severity))],
          sample.pending);

    auto [it, inserted] =
        object_rows.try_emplace(sample.object, statistics.by_object.size());
    if (inserted) {
      statistics.by_object.push_back(
          {.object = sample.object, .name = sample.object_name});
    }
    EventStatistics::ObjectRow& row = statistics.by_object[it->second];
    Count(row.tally, sample.pending);
    row.max_severity = std::max(row.max_severity, sample.severity);
  }

  std::ranges::sort(statistics.by_object,
                    [](const EventStatistics::ObjectRow& a,
                       const EventStatistics::ObjectRow& b) {
                      if (a.tally.count != b.tally.count)
                        return a.tally.count > b.tally.count;
                      return CompareForDisplay(a.name, b.name) < 0;
                    });

  return statistics;
}

std::u16string EventStatisticsBandTitle(scada::aui::SeverityLevel level) {
  if (level == scada::aui::SeverityLevel::kNone)
    return Translate("Routine");
  return SeverityLevelLabel(level);
}

std::u16string FormatEventStatisticsText(const EventStatistics& statistics,
                                         std::u16string_view period) {
  const std::u16string count = Translate("Count");
  const std::u16string unacknowledged = Translate("Unacknowledged");

  std::u16string text;
  if (!period.empty()) {
    text += period;
    text += u"\n\n";
  }

  text +=
      Translate("Severity") + u'\t' + count + u'\t' + unacknowledged + u'\n';
  for (auto level : {scada::aui::SeverityLevel::kCritical,
                     scada::aui::SeverityLevel::kWarning,
                     scada::aui::SeverityLevel::kNone}) {
    AppendTallyRow(text, EventStatisticsBandTitle(level),
                   statistics.band(level));
  }
  AppendTallyRow(text, Translate("Total"), statistics.total);

  text += u'\n';
  text += Translate("Object") + u'\t' + count + u'\t' + unacknowledged + u'\n';
  for (const EventStatistics::ObjectRow& row : statistics.by_object)
    AppendTallyRow(text, row.name, row.tally);

  return text;
}

}  // namespace events
