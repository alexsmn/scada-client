#pragma once

#include "base/any_executor.h"

#include "base/any_executor_timer.h"
#include "scada/date_time.h"

#include <boost/signals2/signal.hpp>

// How long the blink stays in each of its two states.
inline constexpr scada::Duration kBlinkHalfPeriod =
    std::chrono::milliseconds{300};

// The blink state at `time` for a blinker of `half_period`. Blink phase is a
// pure function of wall-clock time rather than a free-running toggle, which
// means every blinker sharing a period is in phase with every other, the phase
// does not depend on when the blinker happened to be constructed, and anything
// that freezes the clock (ScopedMockClockOverride — the screenshot generator
// does exactly this) sees a single, stable state instead of whatever the toggle
// had reached. A capture whose highlight flipped run to run is not
// reproducible, and a screenshot diff is the project's visual-regression
// signal.
//
// Take this overload for any indicator that flashes on its own period. The
// main window's unacknowledged-critical annunciator is the one that exists:
// it flashes at 700ms against this file's 300ms, and it drove its own
// `QTimer` toggle until 2026-09-20 — the same defect this function was written
// to remove, reintroduced later in a widget that never reached for it. The
// caller still needs a timer to SAMPLE the phase; what it must not do is
// advance the phase itself.
bool BlinkPhaseAt(scada::Time time, scada::Duration half_period);

// The blink state at `time`, at the shared kBlinkHalfPeriod.
bool BlinkPhaseAt(scada::Time time);

class BlinkerManager {
 public:
  virtual ~BlinkerManager() = default;

  virtual bool GetState() const = 0;

  using BlinkerCallback = std::function<void(bool state)>;

  virtual boost::signals2::scoped_connection Subscribe(
      const BlinkerCallback& callback) = 0;
};

class BlinkerManagerImpl : public BlinkerManager {
 public:
  explicit BlinkerManagerImpl(AnyExecutor executor);

  // BlinkerManager
  virtual bool GetState() const override { return state_; }
  virtual boost::signals2::scoped_connection Subscribe(
      const BlinkerCallback& callback) override;

 private:
  void Blink();

  bool state_ = BlinkPhaseAt(scada::Now());

  boost::signals2::signal<void(bool state)> signal_;

  AnyExecutorTimer timer_;
};

class Blinker {
 public:
  explicit Blinker(BlinkerManager& blinker_manager);
  virtual ~Blinker();

  Blinker(const Blinker&) = delete;
  Blinker& operator=(const Blinker&) = delete;

  void Start();
  void Stop();

  bool GetState() const;

 protected:
  friend class BlinkerManager;

  // WARNING: Stop from this callback is prohibited. It may cause deletion of
  // BlinkerManager during iteration.
  virtual void OnBlink(bool state) = 0;

 private:
  BlinkerManager& blinker_manager_;

  boost::signals2::scoped_connection connection_;
};
