#pragma once

class SelectionPanelRegistry;

// Contributes the Transmission rule dock: filled for a single
// TransmissionItemType selection, cleared for anything else.
void RegisterTransmissionRuleSelectionPanel(SelectionPanelRegistry& registry);
