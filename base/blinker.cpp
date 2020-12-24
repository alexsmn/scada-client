#include "base/blinker.h"

using namespace std::chrono_literals;

// BlinkerManager

BlinkerManager::BlinkerManager(std::shared_ptr<Executor> executor)
    : timer_{std::move(executor)} {
  timer_.StartRepeating(300ms, [this] { Blink(); });
}

void BlinkerManager::Blink() {
  state_ = !state_;

  for (auto& blinker : blinkers_)
    blinker.OnBlink(state_);
}

// Blinker

Blinker::Blinker(BlinkerManager& blinker_manager)
    : blinker_manager_{blinker_manager} {}

Blinker::~Blinker() {
  Stop();
}

bool Blinker::GetState() const {
  return blinker_manager_.state();
}

void Blinker::Start() {
  blinker_manager_.AddBlinker(*this);
}

void Blinker::Stop() {
  blinker_manager_.RemoveBlinker(*this);
}
