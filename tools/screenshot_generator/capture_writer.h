#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

class QImage;

// Times each capture against the run and against the capture before it, and
// formats the log line `WriteCapture` prints.
//
// It exists because the run's log carried no time at all, so a capture that
// stalled for minutes under load could only be located after the fact from
// output-file mtimes, and a capture that never finished could not be located
// at all (backlog 898). One line per written file, with the gap since the
// previous one, makes the stalled capture the line after the big number.
// Deliberately Qt-free and handed its clock readings, so the arithmetic is
// testable without a widget or a wall clock.
//
// A big gap is not necessarily a stall: the run clock keeps counting while the
// machine sleeps, and an unattended Mac on battery sleeps in stretches of about
// 15 minutes — the size of the gap 898 first measured. So each reading also
// carries an awake clock, which stops during sleep, and a line whose gap was
// mostly sleep says so. Measured 2026-11-03: every multi-minute gap in a run
// across a closed lid was sleep, and one awake run at load 45 had none.
class CaptureTimeline {
 public:
  using Clock = std::chrono::steady_clock;

  // One reading of both clocks. `awake` is time since an arbitrary epoch that
  // excludes system sleep, or nullopt where no such clock is read.
  struct Instant {
    Clock::time_point run;
    std::optional<std::chrono::nanoseconds> awake;
  };

  // Reads both clocks now. `run` is steady_clock, which on macOS is
  // CLOCK_MONOTONIC_RAW and counts sleep; `awake` is CLOCK_UPTIME_RAW, which
  // does not. Off macOS `awake` is nullopt.
  static Instant Now();

  explicit CaptureTimeline(Instant start) : start_{start} {}

  // Records that `filename` finished at `now`, and returns the line to log:
  // `capture <filename> (<outcome>) at <run seconds>s, +<gap seconds>s`,
  // followed by `, <seconds>s of it asleep` when at least a second of the gap
  // was system sleep. The first capture's gap is measured from `start`.
  std::string Record(std::string_view filename,
                     std::string_view outcome,
                     Instant now);

 private:
  const Instant start_;
  std::optional<Instant> previous_;
};

// Writes `image` to `path` as a PNG — unless `path` already holds an image
// whose decoded pixels, size and resolution are identical, in which case the
// existing bytes are kept and only the file's modification time is bumped.
//
// The gallery is tracked so that a changed PNG on the same platform means a
// changed UI (client/CLAUDE.md, "The gallery is tracked, publishing is
// gated"). PNG encoding does not hold up its end of that: two generator
// binaries on one host produced identical pixels in different bytes for ten
// captures (backlog 832), because the encoder's per-row filter choice and its
// compressed stream are not part of the image. Comparing what the file
// *shows* rather than how it is packed is what keeps a re-render that changed
// nothing from showing up as a diff.
//
// The modification time still moves, because `capture_provenance.py
// --produced-since` decides which rows a pass rendered by mtime. A skipped
// write that left it alone would read as a capture this pass never produced.
//
// Every call logs one `CaptureTimeline` line to stderr, success or not.
// Returns false when the image could not be written (or touched).
bool WriteCapture(const QImage& image, const std::filesystem::path& path);
