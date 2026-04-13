#include "fixture_builder.h"

#include "graph_capture.h"
#include "screenshot_config.h"

#include "profile/profile.h"
#include "profile/window_definition.h"

Page MakeScreenshotPage(const std::vector<ScreenshotSpec>& specs,
                        const boost::json::value& json) {
  Page page;
  for (const auto& spec : specs) {
    if (spec.window_type == "Graph")
      page.AddWindow(MakeGraphDefinition(json));
    else
      page.AddWindow(WindowDefinition{spec.window_type});
  }
  return page;
}
