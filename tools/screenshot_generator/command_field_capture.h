#pragma once

struct ScreenshotSpec;

// Renders the top context bar's command/search field on its own.
//
// Standalone, like SaveSeverityTilesScreenshot: the field is the entry point to
// the command palette and carries the marks that say so — the magnifier and the
// platform shortcut hint — so it is worth a capture of its own, independent of
// whichever window happens to host it.
void SaveCommandFieldScreenshot(const ScreenshotSpec& spec);
