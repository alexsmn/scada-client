#pragma once

#include "base/time/time.h"
#include "scada/date_time.h"
#include "scada/node_id.h"

#include <boost/json/value.hpp>

#include <string_view>
#include <vector>

class AddressSpaceImpl;
class Page;
struct ScreenshotSpec;

// The fixture's frozen "now" (the `now` key of screenshot_data.json), or a
// null Time when the fixture does not define one. All fixture history data is
// laid out relative to this instant; captures anchor time-dependent rendering
// (the generator clock, graph ranges) to it so output stays deterministic.
scada::Time FixtureNow(const boost::json::value& json);

// Builds a `Page` whose windows map 1:1 to `specs`. The "Graph" entry
// gets a full `WindowDefinition` from the fixture JSON; everything
// else gets a bare `WindowDefinition{type}`.
Page MakeScreenshotPage(const std::vector<ScreenshotSpec>& specs,
                        const boost::json::value& json);

// Parses a node id as the fixture's `tree` and `nodes` arrays spell it: a
// SCADA-style string ("TIT.211", "85"), or a bare integer meaning ns=1.
scada::NodeId ParseJsonChildNodeId(const boost::json::value& child);

// Parses one node id string as the fixture spells it: SCADA-style ("TIT.211")
// or a bare decimal, which names a standard ns=0 node ("85" is Objects).
scada::NodeId ParseFixtureNodeIdString(std::string_view s);

// Adds the JSON's ns=1 instance nodes to `address_space` on top of the
// standard SCADA tree that ScadaTestAddressSpace builds in code.
// Each entry becomes a Variable (class="variable") or Object
// (class="object") parented under the node specified by the JSON `tree`
// map, joined via `Organizes`. Variables get their `base_value` written
// in as the static AttributeId::Value. ns=0 / ns=7 entries in the JSON
// are skipped — they already exist in ScadaTestAddressSpace.
void PopulateFixtureNodes(AddressSpaceImpl& address_space,
                          const boost::json::value& json);

// Projects the fixture's UserType instances onto the OPC UA standard user
// model, which is what the Users grid actually reads: the
// UserManagement.Users property (Part 18 §5.2.2) and the RoleSet membership
// rules that carry each account's Roles.
//
// Derived from the same instances rather than authored separately, so the
// fixture keeps ONE source of truth — and derived the same way the server's
// one-shot migration is (an account's AccessRights bits become membership of
// Operator and/or ConfigureAdmin), so the capture shows what a migrated
// deployment really looks like.
//
// Must run after PopulateFixtureNodes.
void ProjectFixtureUsersOntoStandardModel(AddressSpaceImpl& address_space,
                                          const boost::json::value& json);
