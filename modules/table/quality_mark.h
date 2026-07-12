#pragma once

#include "aui/severity_colors.h"
#include "scada/qualifier.h"

// Maps a live value's SCADA quality flags to the coarse good/uncertain/bad
// bucket the reshell quality marks render. Hard comms/config failures
// (bad/failed/offline/misconfigured) read as bad; a stale — present but
// not-updated — value reads as uncertain; everything else reads as good. This
// mirrors the Explorer status-dot intent (`VisibleNodeModel::GetStatusColor`,
// `IsBad`/`IsAlerting`) applied to a table row's current value, so the tree and
// the grid resolve quality the same way. Pure and header-light so the mapping
// can be unit-tested without Qt.
scada::aui::Quality QualityFromQualifier(scada::Qualifier qualifier);
