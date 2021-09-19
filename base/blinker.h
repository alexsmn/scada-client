#pragma once

#include "base/executor_timer.h"

#include <boost/signals2/signal.hpp>

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
  explicit BlinkerManagerImpl(std::shared_ptr<Executor> executor);

  // BlinkerManager
  virtual bool GetState() const override { return state_; }
  virtual boost::signals2::scoped_connection Subscribe(
      const BlinkerCallback& callback) override;

 private:
  void Blink();

  bool state_ = false;

  boost::signals2::signal<void(bool state)> signal_;

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

  boost::signals2::scoped_connection connection_;
};
