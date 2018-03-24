#pragma once

class BlinkManager;

class Blinker {
 public:
  Blinker() {}
	virtual ~Blinker();

  Blinker(const Blinker&) = delete;
  Blinker& operator=(const Blinker&) = delete;

	void Start();
	void Stop();

	static bool GetState();

protected:
  friend class BlinkerManager;

  // WARNING: Stop from this callback is prohibited. It may cause deletion of
  // BlinkerManager during iteration.
	virtual void OnBlink(bool state) = 0;
};
