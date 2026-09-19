#pragma once

struct ScreenshotSpec;

// The three shell strips the workbench draws around the workspace, each
// rendered on its own.
//
// Standalone, like SaveCommandFieldScreenshot: every one of them is a thin band
// inside a 1920px window, unreadable in any whole-window capture even though
// `workbench-window.png` contains all three. Building the widget directly also
// keeps each picture independent of whichever view happens to be open.
//
// The two rail strips come from one `ActivityBar` each, populated in the single
// zone the capture is of. The bar has three zones — modes, pages, then the
// pinned utilities behind a stretch — and leaving the other two empty is what
// crops the picture to the subject, since a QToolBar lays out only what it
// holds. That is the Qt counterpart of the web generator clipping its rail
// capture to a selector.

// The rail's page band: the profile's pages plus the "+" that adds one.
void SavePagesScreenshot(const ScreenshotSpec& spec);

// The rail's third zone: the utilities pinned at its foot.
void SaveRailUtilitiesScreenshot(const ScreenshotSpec& spec);

// The context bar's breadcrumb — page, view, and the current selection.
void SaveBreadcrumbScreenshot(const ScreenshotSpec& spec);
