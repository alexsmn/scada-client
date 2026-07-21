#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "modules/events/event_table_model.h"
#include "modules/events/qt/event_filter_bar.h"

#include <functional>
#include <optional>
#include <span>
#include <vector>

class QWidget;

// Inputs for the journal's Areas sidebar (Qt): the mockup's left rail listing
// "All areas" plus the top-level areas, each with its unacknowledged count,
// driving the journal's area filter. Opt-in reshell chrome; the journal only.
//
// The sidebar is decoupled from the journal model behind callbacks so the
// widget is testable: `browse_areas` enumerates the areas asynchronously
// (BrowseEventAreas in production), `counts` supplies the unacknowledged
// counts for the listed areas
// (EventTableModel::CountUnacknowledgedByArea), and `model` provides the
// change notifications that keep the counts current.
struct EventAreaSidebarContext {
  AnyExecutor executor;
  scada::aui::TableModel& model;
  std::function<Awaitable<std::vector<EventAreaEntry>>()> browse_areas;
  std::function<EventTableModel::AreaCounts(std::span<const scada::NodeId>)>
      counts;
  // The chosen filter scope: an area node, or nullopt for "All areas".
  std::function<void(const std::optional<scada::NodeId>&)> on_area;
};

// Builds the sidebar. The returned widget owns its rows and invokes `on_area`
// on selection changes.
QWidget* MakeEventAreaSidebar(EventAreaSidebarContext context);
