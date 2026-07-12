#include "main_window/overview_page.h"

#include "aui/translation.h"
#include "profile/window_definition.h"

#include <string_view>

Page MakeOverviewPage() {
  Page page;
  page.title = Translate("Overview");

  // Dominant trend.
  page.AddWindow(WindowDefinition{std::string_view{"Graph"}});

  // Active-alarm table: the event journal in "Current" mode surfaces only the
  // unacknowledged (actionable) alarms.
  WindowDefinition alarms{std::string_view{"EventJournal"}};
  alarms.AddItem("mode", "Current");
  page.AddWindow(alarms);

  return page;
}
