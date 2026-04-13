#pragma once

#include <boost/json/value.hpp>

#include <vector>

class AddressSpaceImpl;
class Page;
struct ScreenshotSpec;

// Builds a `Page` whose windows map 1:1 to `specs`. The "Graph" entry
// gets a full `WindowDefinition` from the fixture JSON; everything
// else gets a bare `WindowDefinition{type}`.
Page MakeScreenshotPage(const std::vector<ScreenshotSpec>& specs,
                        const boost::json::value& json);

// Adds the JSON's ns=1 instance nodes to `address_space` on top of the
// standard SCADA tree that AddressSpaceImpl3 builds in its constructor.
// Each entry becomes a Variable (class="variable") or Object
// (class="object") parented under the node specified by the JSON `tree`
// map, joined via `Organizes`. Variables get their `base_value` written
// in as the static AttributeId::Value. ns=0 / ns=7 entries in the JSON
// are skipped — they already exist in AddressSpaceImpl3.
void PopulateFixtureNodes(AddressSpaceImpl& address_space,
                          const boost::json::value& json);
