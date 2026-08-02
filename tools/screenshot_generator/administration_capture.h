#pragma once

struct ScreenshotSpec;

// Renders the Administration explorer pane (the left region of
// `docs/product/ui-mockups/screens/users-admin.html`).
//
// Standalone chrome rather than an opened page view: the pane derives its rows
// from the shell's live command resolution, which the headless generator has
// no shell for. The capture therefore supplies the section set directly — every
// section the client knows how to open — which is what an administrator with
// full rights sees.
void SaveAdministrationScreenshot(const ScreenshotSpec& spec);
