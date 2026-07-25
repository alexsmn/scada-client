#pragma once

#include "base/any_executor.h"

#include "base/any_executor_timer.h"
#include "scada/date_time.h"

#include <boost/signals2/signal.hpp>

// How long the blink stays in each of its two states.
inline constexpr scada::Duration kBlinkHalfPeriod =
    std::chrono::milliseconds{300};

// The blink state at `time`. Blink phase is a pure function of wall-clock time
// rather than a free-running toggle, which means every blinker in the process
// is in phase with every other, the phase does not depend on when the manager
// happened to be constructed, and anything that freezes the clock
// (ScopedMockClockOverride — the screenshot generator does exactly this) sees a
// single, stable state instead of whatever the toggle had reached. A capture
// whose highlight flipped run to run is not reproducible, and a screenshot
// diff is the project's visual-regression signal.
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
