#pragma once

class SelectionPanelRegistry;

// Contributes the Access rights dock: filled for a single UserType selection,
// cleared for anything else.
void RegisterUserAccessSelectionPanel(SelectionPanelRegistry& registry);
