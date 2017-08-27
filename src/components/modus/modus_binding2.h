#pragma once

#include "base/blinker.h"
#include "components/modus/modus_style2.h"
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

class ModusBinding2 : private rt::TimedDataDelegate,
                      private ModusStyle2::AnimationObserver {
 public:
  class Delegate {
   public:
    virtual void SchedulePaintShape(modus::Shape& shape) = 0;
  };

  ModusBinding2(Delegate& delegate, TimedDataService& timed_data_service, modus::Shape& shape, const std::wstring& binding);
  ~ModusBinding2();

  const rt::TimedDataSpec& data_point() const { return data_point_; }

  void Paint(Gdiplus::Graphics& graphics, bool background);

 private:
  bool Update();

  bool SetStyles(unsigned styles);

  // rt::TimedDataDelegate
  virtual void OnPropertyChanged(rt::TimedDataSpec& spec,
                                 const rt::PropertySet& properties) override;
  virtual void OnEventsChanged(rt::TimedDataSpec& spec,
                               const events::EventSet& events) override;

  // ModusStyle2::AnimationObserver
	virtual void OnAnimationStep() override;

  Delegate& delegate_;
  modus::Shape& shape_;

  std::string property_name_;
  rt::TimedDataSpec data_point_;

  unsigned styles_;
};