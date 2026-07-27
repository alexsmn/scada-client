#pragma once

struct ScreenshotSpec;

// Renders the protocol/request debugger — debugger.html — and saves it under
// `GetOutputDir() / spec.filename`.
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
