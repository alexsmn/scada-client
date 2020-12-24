#pragma once

#include "base/observer_list.h"
#include "base/executor_timer.h"

class Blinker;

class BlinkerManager {
 public:
  explicit BlinkerManager(std::shared_ptr<Executor> executor);

  bool state() const { return state_; }

  void AddBlinker(Blinker& blinker) { blinkers_.AddObserver(&blinker); }
  void RemoveBlinker(Blinker& blinker) { blinkers_.RemoveObserver(&blinker); }

 private:
  void Blink();

  bool state_ = false;
  base::ObserverList<Blinker> blinkers_;

  ExecutorTimer timer_;
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
};
