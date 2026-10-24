#include "print/service/print_header.h"

#include "aui/translation.h"
#include "base/format_time.h"
#include "base/u16format.h"
#include "base/utf_convert.h"

namespace {

std::u16string FormatHeaderTime(scada::Time time) {
  return UtfConvert<char16_t>(
      FormatTime(time, TIME_FORMAT_DATE | TIME_FORMAT_TIME));
}

}  // namespace

PrintHeader MakePrintHeader(std::u16string title,
                            const std::optional<scada::TimeRange>& period,
                            std::u16string description,
                            scada::Time printed_at) {
  PrintHeader header{.title = std::move(title)};

  if (period) {
    header.lines.push_back(u16format(Translate("Period: {} – {}"),
                                     FormatHeaderTime(period->first),
                                     FormatHeaderTime(period->second)));
  }

  if (!description.empty())
    header.lines.push_back(std::move(description));

  header.lines.push_back(
      u16format(Translate("Printed: {}"), FormatHeaderTime(printed_at)));

  return header;
}
