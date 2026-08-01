#include "main_window/page_icons.h"

#include "base/no_destructor.h"

#include <algorithm>
#include <vector>

namespace {

// The pickable set. Every key here must have an arm in the Qt rail's
// `PageGlyph` — `activity_bar_unittest.cpp` asserts the two stay in step, so a
// key added without a glyph fails the build's tests rather than shipping a
// page that draws nothing.
const std::vector<PageIcon>& PageIconTable() {
  static const scada::base::NoDestructor<std::vector<PageIcon>> icons{
      std::vector<PageIcon>{
          {.key = "overview", .label = "Overview"},
          {.key = "alarms", .label = "Alarms"},
          {.key = "trend", .label = "Trend"},
          {.key = "substation", .label = "Substation"},
          {.key = "table", .label = "Table"},
          {.key = "objects", .label = "Objects"},
          {.key = "devices", .label = "Devices"},
          {.key = "devicelog", .label = "Device log"},
          {.key = "transmission", .label = "Transmission"},
          {.key = "files", .label = "Files"},
          {.key = "report", .label = "Report"},
          {.key = "settings", .label = "Settings"},
      }};
  return *icons;
}

}  // namespace

std::span<const PageIcon> GetPageIcons() {
  return PageIconTable();
}

bool IsPageIconKey(std::string_view key) {
  if (key.empty())
    return false;
  return std::ranges::find(PageIconTable(), key, &PageIcon::key) !=
         PageIconTable().end();
}
