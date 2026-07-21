#include "main_window/overview_page.h"

#include "aui/translation.h"
#include "profile/window_definition.h"

#include <string_view>

Page MakeOverviewPage() {
  Page page;
  page.title = Translate("Overview");

  // Dominant trend.
  WindowDefinition& trend =
      page.AddWindow(WindowDefinition{std::string_view{"Graph"}});

  // Active-alarm table: the event journal in "Current" mode surfaces only the
  // unacknowledged (actionable) alarms.
  WindowDefinition alarms_definition{std::string_view{"EventJournal"}};
  alarms_definition.AddItem("mode", "Current");
  WindowDefinition& alarms = page.AddWindow(alarms_definition);

  // The mockup's cockpit proportions: the trend dominates (~two thirds of the
  // workspace) with the actionable-alarm strip under it. `pos` is the top
  // block's percentage share of the splitter.
  PageLayoutBlock& main = page.layout.main;
  main.split(/*horizontally=*/true);
  main.pos = 65;
  main.top().central = true;
  main.top().add(trend.id);
  main.top().active_window = trend.id;
  main.bottom().add(alarms.id);
  main.bottom().active_window = alarms.id;

  return page;
}
