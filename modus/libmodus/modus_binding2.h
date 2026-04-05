#pragma once

#include "base/blinker.h"
#include "modus/libmodus/modus_style2.h"
#include "timed_data/timed_data_spec.h"

namespace {
const int kModusBindingInflate = 3;
}

namespace Gdiplus {
class Graphics;
}

namespace modus {
class Shape;
}

class ModusView2;
class TimedDataService;

class ModusBinding2 : private ModusStyle2::AnimationObserver {
 public:
  class Delegate {
   public:
    virtual void SchedulePaintShape(modus::Shape& shape) = 0;
  };

  ModusBinding2(Delegate& delegate,
                modus::Shape& shape,
                const std::wstring& binding,
                TimedDataService& timed_data_service);
  ~ModusBinding2();

  const TimedDataSpec& data_point() const { return data_point_; }

  void Paint(Gdiplus::Graphics& graphics, bool background);

 private:
  bool Update();

  bool SetStyles(unsigned styles);

  // ModusStyle2::AnimationObserver
  virtual void OnAnimationStep() override;

  Delegate& delegate_;
  modus::Shape& shape_;

  std::string property_name_;
  TimedDataSpec data_point_;

  unsigned styles_;
};
