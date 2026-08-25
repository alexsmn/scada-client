#pragma once

#include <chrono>
#include <utility>

namespace scada::screenshot_generator {

// Consecutive identical frames that count as settled, and the event-loop time
// between them.
inline constexpr int kSettledFrames = 3;
inline constexpr auto kFrameInterval = std::chrono::milliseconds{120};

// Frames compared before the loop gives up.
//
// An iteration budget, not a wall-clock deadline, and the distinction is the
// whole point of this constant. The loop's currency is *comparisons*: it
// settles when kSettledFrames consecutive grabs match. A wall-clock deadline
// buys fewer comparisons exactly when the machine is busy — each pass pays a
// repaint, a grab, and a QTimer that fires late under load — so a capture that
// settles comfortably on an idle machine could exhaust a 10 s budget after a
// handful of frames while peer builds were running, and then fail claiming the
// widget was animating. Measured 2026-08-23: the same binary took 18.9 s idle
// and blew a 300 s ctest timeout under load (task 468).
//
// Counting frames makes load cost wall time, which is honest, instead of
// coverage, which is not. 80 frames is what the old 10 s deadline bought on an
// idle machine, so an unloaded run behaves as before.
inline constexpr int kMaxFrames = 80;

// Whether the loop settled, and how many frames it grabbed getting there.
struct SettleResult {
  bool settled = false;
  int frames = 0;
};

// Grabs frames until the last `settled_frames` are identical, or `max_frames`
// have been compared.
//
// `grab` returns the current frame (compared with ==); `pump` advances whatever
// produces the next one. Deliberately Qt-free and free of any clock, so the
// budget's shape can be tested without a widget, an event loop, or a machine
// that happens to be idle.
template <class Grab, class Pump>
SettleResult RunSettleLoop(Grab&& grab,
                           Pump&& pump,
                           int max_frames = kMaxFrames,
                           int settled_frames = kSettledFrames) {
  using Frame = decltype(grab());

  Frame previous{};
  bool has_previous = false;
  int repeats = 0;

  for (int frames = 1; frames <= max_frames; ++frames) {
    Frame frame = grab();
    repeats = (has_previous && frame == previous) ? repeats + 1 : 0;
    previous = std::move(frame);
    has_previous = true;

    if (repeats >= settled_frames) {
      return {.settled = true, .frames = frames};
    }
    if (frames == max_frames) {
      break;
    }
    pump();
  }
  return {.settled = false, .frames = max_frames};
}

}  // namespace scada::screenshot_generator
