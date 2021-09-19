#include "base/blinker.h"

using namespace std::chrono_literals;

// BlinkerManagerImpl

BlinkerManagerImpl::BlinkerManagerImpl(std::shared_ptr<Executor> executor)
    : timer_{std::move(executor)} {
  timer_.StartRepeating(300ms, [this] { Blink(); });
}

void BlinkerManagerImpl::Blink() {
  state_ = !state_;
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
  connection_ =
      blinker_manager_.Subscribe([this](bool state) { OnBlink(state); });
}

void Blinker::Stop() {
  connection_.disconnect();
}
