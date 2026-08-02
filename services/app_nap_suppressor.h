#pragma once

// Keeps the process out of macOS App Nap for as long as the object lives.
//
// The Qt client dispatches every asio completion handler, coroutine
// continuation and timer callback from one 10 ms `QTimer` on the GUI thread
// (`MessageLoopQt`, driven from `app/qt/main.cpp`). macOS puts a process whose
// windows are not visible into App Nap, and App Nap coalesces that timer down
// to multi-second granularity. Sockets themselves keep running on the asio
// thread, but nothing in the client observes them: ping RTT climbs into tens of
// seconds, live values age into bad quality, and events queue until the app is
// brought forward again, when they arrive in a burst. Measured 2026-08-01
// against the GCP demo: a 22.75 s worst-case stall at `timer:Tier5`, versus
// 0.97 s for the same build holding this assertion.
//
// The suppressor holds an `NSProcessInfo` activity assertion with
// `NSActivityUserInitiatedAllowingIdleSystemSleep`: it clears the App Nap bits
// so the timer keeps its requested period, but deliberately does *not* keep the
// machine awake — an operator who walks away should still get idle sleep, and a
// sleeping Mac drops the session anyway. It is a no-op on every other platform.
//
// See docs/client/message-loop.md for the full analysis, and §8 there for the
// proposed event-driven pump that removes the polling this works around. That
// change would not make this assertion redundant: an event-driven pump still
// runs on the GUI thread, which App Nap still throttles.
class AppNapSuppressor {
 public:
#if defined(__APPLE__)
  AppNapSuppressor();
  ~AppNapSuppressor();
#else
  AppNapSuppressor() = default;
  ~AppNapSuppressor() = default;
#endif

  AppNapSuppressor(const AppNapSuppressor&) = delete;
  AppNapSuppressor& operator=(const AppNapSuppressor&) = delete;

  // True while an assertion is actually held. Always false off macOS, and false
  // on macOS if `beginActivityWithOptions:` returned nothing.
  bool active() const { return token_ != nullptr; }

 private:
  // Retained `id<NSObject>` activity token on macOS; always null elsewhere.
  void* token_ = nullptr;
};
