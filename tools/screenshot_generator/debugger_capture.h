#pragma once

struct ScreenshotSpec;

// Renders the session-request debugger and saves it under
// `OutputPathFor(spec.filename)`.
//
// This is the client<->server request trace, not the device protocol frame
// trace in docs/product/ui-mockups/screens/device-protocol-trace.html — that is a
// separate, unbuilt surface planned for the device log view.
//
// Standalone like SaveBulkCreateScreenshot: the debugger is not a registered
// view (no `WindowInfo`), it is a `--debug`-gated window that `Debugger::Open()`
// creates for itself, so nothing in the ordinary view sweep can reach it.
//
// It builds the *real* `Debugger` over the real `RequestTableModel`, and feeds
// it through the only seam the model has: `SessionService::GetSessionDebugger()`
// returns a fixture `SessionDebugger` that replays a handful of request events.
// That needs no production change — `SubscribeRequestEvents` is already the
// interface the model subscribes through — and it exercises the real event
// handling, filtering and duration formatting rather than a copy of them.
void SaveDebuggerScreenshot(const ScreenshotSpec& spec);
