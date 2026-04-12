#include "fixture_builder.h"

#include "graph_capture.h"
#include "screenshot_config.h"

#include "base/time_utils.h"
#include "profile/profile.h"
#include "profile/window_definition.h"
#include "timed_data/timed_data_service_fake.h"

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

std::unique_ptr<FakeTimedDataService> MakeLocalTimedDataService(
    const boost::json::value& json) {
  auto service = std::make_unique<FakeTimedDataService>();
  const auto now = base::Time::Now();

  for (const auto& jtd : json.at("timed_data").as_array()) {
    auto formula = std::string(jtd.at("formula").as_string());
    const auto& jvalues = jtd.at("values").as_array();

    auto td = service->AddTimedData(formula);
    auto count = static_cast<int>(jvalues.size());
    for (int i = 0; i < count; ++i) {
      auto time = now - base::TimeDelta::FromMinutes(30 * (count - i));
      td->data_values.push_back(
          scada::DataValue{scada::Variant{jvalues[i].to_number<double>()},
                           {},
                           time,
                           time});
    }
    td->ready_ranges.push_back(
        {now - base::TimeDelta::FromMinutes(30 * count), now});
  }

  return service;
}
