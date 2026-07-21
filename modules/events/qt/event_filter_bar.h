#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "base/time_range.h"
#include "scada/node_id.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

class QWidget;
class NodeService;

// Inputs and callbacks for the event-journal filter strip (Qt).
//
// The bar is the discoverable, always-visible surfacing of the journal filters,
// complementing the right-click context menu (`EventMenuModel`). It hosts an
// "Unacknowledged only" toggle, a minimum-severity control and a Period
// selector; the area scope lives in the Areas sidebar beside the journal
// (`MakeEventAreaSidebar`). It is opt-in reshell chrome; the caller gates it
// on the active UX theme and only builds it for the full historical journal
// (where the time range is meaningful), not the docked current-events panel.
//
// The Period selector offers the fixed quick-pick ranges; an arbitrary/custom
// range (via the toolbar or context menu) is still reflected when it matches a
// preset and otherwise leaves the selector unselected.
struct EventFilterBarContext {
  // Async work host (kept for parity with the sidebar; the bar itself runs no
  // async work today).
  AnyExecutor executor;
  NodeService& node_service;

  // Current filter state reflected in the controls.
  bool unacknowledged_only = false;
  unsigned severity_min = 0;
  unsigned severity_max = 0;
  TimeRange time_range;

  // Invoked on user changes. Any callback may be empty.
  std::function<void(bool)> on_unacknowledged_only;
  std::function<void(unsigned)> on_severity_min;
  std::function<void(const TimeRange&)> on_time_range;
};

// Builds the filter strip. The returned widget owns its controls and invokes
// the callbacks on user changes.
QWidget* MakeEventFilterBar(EventFilterBarContext context);

// The fixed period ranges the bar offers, in display order. Mirrors the
// toolbar's quick-pick ranges so a range set there reflects onto a preset.
// Exposed for testing.
const std::vector<TimeRange>& EventPeriodRanges();

// Index into `EventPeriodRanges()` whose range equals `range`, or -1 when none
// matches (an arbitrary/custom range that has no quick-pick). Exposed for
// testing.
int EventPeriodPresetIndex(const TimeRange& range);

// A top-level area filterable in the journal: an object grouping (a folder or
// object, not a leaf data item) directly under the address space's DataItems
// root — the same root the object tree browses.
struct EventAreaEntry {
  scada::NodeId node_id;
  std::u16string name;
};

// Enumerates the top-level areas — the immediate object-grouping children of
// the DataItems root, leaf data items excluded — as {node_id, display name}.
// This is exactly what the Area selector lists; the bar populates the dropdown
// from it asynchronously. Exposed so the enumeration can be verified against a
// real NodeService.
Awaitable<std::vector<EventAreaEntry>> BrowseEventAreas(
    NodeService& node_service);
