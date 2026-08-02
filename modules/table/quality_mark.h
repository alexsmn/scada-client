#pragma once

#include "aui/severity_colors.h"
#include "scada/data_value.h"
#include "scada/qualifier.h"

#include <optional>

// Maps a live value's SCADA quality flags to the coarse good/uncertain/bad
// bucket the reshell quality marks render. Hard comms/config failures
// (bad/failed/offline/misconfigured) read as bad; a stale — present but
// not-updated — value reads as uncertain; everything else reads as good. This
// mirrors the Explorer status-dot intent (`VisibleNodeModel::GetStatusColor`,
// `IsBad`/`IsAlerting`) applied to a table row's current value, so the tree and
// the grid resolve quality the same way. Pure and header-light so the mapping
// can be unit-tested without Qt.
scada::aui::Quality QualityFromQualifier(scada::Qualifier qualifier);

// The same mapping for a whole value, with one thing the Qualifier alone
// cannot express: std::nullopt when nothing has ever been delivered for the
// row. A default-constructed Qualifier is zero, and zero is *not* BAD, so
// mapping it through QualityFromQualifier reports Good — which is how a row
// bound to an unresolvable node id came to show a green "Good" mark beside a
// permanently empty Value cell. Prefer this overload wherever the DataValue is
// at hand; the Inspector applies the identical rule
// (`InspectorQualityBandFor(const DataValue&)`).
std::optional<scada::aui::Quality> QualityFromValue(
    const scada::DataValue& value);
