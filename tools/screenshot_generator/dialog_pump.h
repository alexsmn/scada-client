#pragma once

#include "screenshot_wait.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <gtest/gtest.h>

#include <chrono>
#include <exception>
#include <memory>

namespace scada::screenshot_generator {

// Pumps the Qt event loop until `predicate()` holds or `timeout` elapses.
//
// Uses the single-argument processEvents(WaitForMoreEvents) form on purpose:
// the two-argument processEvents(flags, ms) overload strips
// QEventLoop::WaitForMoreEvents and returns as soon as the queue drains, and
// on the macOS (Cocoa) dispatcher a drained no-wait pass never activates
// QTimer timers — which starves MessageLoopQt's 10 ms task-queue timer, so
// CoSpawn'd dialog coroutines never get their initial resume. The
// single-argument form blocks until the next event (the 10 ms timer at the
// latest), so the executor's tasks actually run.
template <class Predicate>
bool PumpEventsUntil(Predicate&& predicate, std::chrono::milliseconds timeout) {
  QElapsedTimer timer;
  timer.start();
  while (!predicate()) {
    if (timer.elapsed() >= timeout.count())
      return false;
    QApplication::processEvents(QEventLoop::WaitForMoreEvents);
  }
  return true;
}

// Pumps the Qt event loop for a fixed duration, letting queued executor
// tasks and async value updates land.
inline void PumpEventsFor(std::chrono::milliseconds duration) {
  PumpEventsUntil([] { return false; }, duration);
}

// Waits for a dialog awaitable started with StartAwaitable to finish, after
// the capture has grabbed and rejected the dialog.
//
// Swallows the exception the awaitable ends with: captures close dialogs
// through reject(), so a modal-dialog awaitable normally finishes by throwing
// the cancellation exception. What this is really waiting for is the
// coroutine frame to be gone before the per-call stubs it borrowed —
// transport factory, task manager, dialog service — leave scope.
template <class T>
void WaitForDialogCompletion(
    const std::shared_ptr<AwaitableResult<T>>& result) {
  if (!PumpEventsUntil([&] { return result->done; }, std::chrono::seconds{4}))
    ADD_FAILURE() << "Dialog coroutine did not complete";

  try {
    RethrowAwaitableError(result);
  } catch (const std::exception&) {
  }
}

}  // namespace scada::screenshot_generator
