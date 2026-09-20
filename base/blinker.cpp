#include "base/blinker.h"

using namespace std::chrono_literals;

namespace {

// The phase is sampled rather than toggled, so the tick has to be short enough
// to notice every flip: at exactly one flip per tick the sampling would alias
// and the blink could stall or double up.
constexpr scada::Duration kBlinkTick = kBlinkHalfPeriod / 2;

}  // namespace

bool BlinkPhaseAt(scada::Time time, scada::Duration half_period) {
  const int64_t ticks = time.time_since_epoch().count();
  const int64_t period = half_period.count();

  // Floor division, not the built-in truncating one: `/` rounds toward zero, so
  // the half-period just before the epoch and the one just after would both
  // land on quotient 0 and share a phase — one double-length blink straddling
  // 1970. Parity itself is sign-safe (a negative odd remainder is still
  // non-zero), so only the division needs correcting.
  int64_t half_periods = ticks / period;
  if (ticks < 0 && ticks % period != 0)
    --half_periods;

  return half_periods % 2 != 0;
}

bool BlinkPhaseAt(scada::Time time) {
  return BlinkPhaseAt(time, kBlinkHalfPeriod);
}

// BlinkerManagerImpl

BlinkerManagerImpl::BlinkerManagerImpl(AnyExecutor executor)
    : timer_{std::move(executor)} {
  timer_.StartRepeating(
      std::chrono::duration_cast<std::chrono::milliseconds>(kBlinkTick),
      [this] { Blink(); });
}

void BlinkerManagerImpl::Blink() {
  const bool state = BlinkPhaseAt(scada::Now());
  // Only a real phase change is worth a repaint; under a frozen clock this
  // never fires, which is what keeps a capture still.
  if (state == state_)
    return;

  state_ = state;
  signal_(state_);
}

boost::signals2::scoped_connection BlinkerManagerImpl::Subscribe(
    const BlinkerCallback& callback) {
  return signal_.connect(callback);
}

// Blinker

Blinker::Blinker(BlinkerManager& blinker_manager)
    : blinker_manager_{blinker_manager} {}

Blinker::~Blinker() {
  Stop();
}

bool Blinker::GetState() const {
  return blinker_manager_.GetState();
}

void Blinker::Start() {
  if (!connection_.connected()) {
    connection_ =
        blinker_manager_.Subscribe([this](bool state) { OnBlink(state); });
  }
}

void Blinker::Stop() {
  connection_.disconnect();
}
