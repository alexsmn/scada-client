#pragma once

class SelectionPanelRegistry;

// Contributes the Device diagnostics dock: filled for a single DeviceType
// selection, cleared for anything else.
void RegisterDeviceDiagnosticsSelectionPanel(SelectionPanelRegistry& registry);
