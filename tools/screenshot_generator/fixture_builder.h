#pragma once

#include <boost/json/value.hpp>

#include <memory>
#include <vector>

class FakeTimedDataService;
class Page;
struct ScreenshotSpec;

// Builds a `Page` whose windows map 1:1 to `specs`. The "Graph" entry
// gets a full `WindowDefinition` from the fixture JSON; everything
// else gets a bare `WindowDefinition{type}`.
Page MakeScreenshotPage(const std::vector<ScreenshotSpec>& specs,
                        const boost::json::value& json);

// Constructs a FakeTimedDataService and populates it from the
// `timed_data` section of the fixture JSON.
std::unique_ptr<FakeTimedDataService> MakeLocalTimedDataService(
    const boost::json::value& json);
