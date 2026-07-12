#pragma once

#include <functional>

class QWidget;

// Builds the event-journal filter strip (Qt): an "Unacknowledged only" toggle
// and a minimum-severity control (0 = all). This is the cross-platform-facing
// surfacing of the journal filters — the legacy event context menu
// (IDR_EVENT_POPUP) is Windows-only. It is opt-in reshell chrome; the caller
// gates it on the active UX theme. The returned widget owns its controls and
// invokes the callbacks on user changes.
QWidget* MakeEventFilterBar(bool unacknowledged_only,
                            unsigned severity_min,
                            unsigned severity_max,
                            std::function<void(bool)> on_unacknowledged_only,
                            std::function<void(unsigned)> on_severity_min);
