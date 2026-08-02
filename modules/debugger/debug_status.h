#pragma once

#include "modules/debugger/request_table_model.h"
#include "scada/session_debugger.h"

#include <string>

// The status band of a debugger request/response row. A running request reads neutral
// (accent), a succeeded one good, a failed one bad. Pure, so it is unit-testable
// without Qt.
enum class DebugStatus { kRunning, kOk, kError };

DebugStatus DebugStatusFor(scada::SessionDebugger::RequestPhase phase);

// True when `request` matches the trace filter `query` (case-insensitive
// substring over the title, or an exact request-id match). An empty query
// matches everything. Pure.
bool DebugRequestMatches(const RequestTableModel::Request& request,
                         const std::u16string& query);
