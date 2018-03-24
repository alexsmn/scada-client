#pragma warning(disable:4355)
#include "base/timer/timer.h"
#include "base/blinker.h"

#include <cassert>
#include <set>

// BlinkerManager

class BlinkerManager {
 public:
  BlinkerManager()
      : state_(false) {
    timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(BLINK_PERIOD),
        this, &BlinkerManager::Blink);
  }

  bool state() const { return state_; }

  void AddBlinker(Blinker& blinker) {
    blinkers_.insert(&blinker);
  }

  bool RemoveBlinker(Blinker& blinker) {
    blinkers_.erase(&blinker);
    return blinkers_.empty();
  }

private:
  void Blink() {
    state_ = !state_;

    for (BlinkerSet::iterator i = blinkers_.begin(); i != blinkers_.end(); ++i)
      (*i)->OnBlink(state_);
  }

  typedef std::set<Blinker*> BlinkerSet;

  bool state_;
  BlinkerSet blinkers_;
  
  base::RepeatingTimer timer_;

  static unsigned BLINK_PERIOD;

  DISALLOW_COPY_AND_ASSIGN(BlinkerManager);
};

unsigned BlinkerManager::BLINK_PERIOD = 300;

static BlinkerManager* s_blinker_manager = NULL;

// Blinker

Blinker::~Blinker() {
  Stop();
}

bool Blinker::GetState() {
  return s_blinker_manager && s_blinker_manager->state();
}

void Blinker::Start() {
  if (!s_blinker_manager)
    s_blinker_manager = new BlinkerManager;
  s_blinker_manager->AddBlinker(*this);
}

void Blinker::Stop() {
  if (!s_blinker_manager)
    return;

  if (s_blinker_manager->RemoveBlinker(*this)) {
    delete s_blinker_manager;
    s_blinker_manager = NULL;
  }
}
