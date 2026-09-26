#pragma once

#include "scada/status.h"

#include <string>

// The operator-facing description of `status_code`, in the display locale.
//
// This table is the only place status codes are worded: core carries the
// symbolic names alone (`scada::SetStatusTextProvider`), and this function is
// what the client installs there, so every `ToString16(status)` in shared code
// renders through it. Codes the table does not know fall back to a generic
// success or error sentence by severity.
std::u16string StatusText(scada::StatusCode status_code);

// Installs `StatusText` as the process-wide `scada::StatusTextProvider`.
void InstallStatusText();
