#pragma once

#include "base/blinker.h"
#include "base/lifetime.h"
#include "modus/libmodus/modus_style2.h"

#include "timed_data/timed_data_spec.h"
#include <boost/signals2/connection.hpp>
#include <map>

namespace {
const int kModusBindingInflate = 3;
}

namespace Gdiplus {
class Graphics;
}

namespace scada::modus {
class Shape;
}

class ModusView2;
class TimedDataService;

class ModusBinding2 {
 public:
  class Delegate {
   public:
    virtual void SchedulePaintShape(scada::modus::Shape& shape) = 0;
  };

  ModusBinding2(Delegate& delegate,
                scada::modus::Shape& shape,
                const std::wstring& binding,
                TimedDataService& timed_data_service);
  ~ModusBinding2();

  const TimedDataSpec& data_point() const SCADA_LIFETIME_BOUND {
    return data_point_;
  }

  void Paint(Gdiplus::Graphics& graphics, bool background);

 private:
  bool Update();

  bool SetStyles(unsigned styles);

  void OnAnimationStep();

  Delegate& delegate_;
  scada::modus::Shape& shape_;

  std::string property_name_;
  TimedDataSpec data_point_;

  unsigned styles_;

  // Animation-step subscriptions of the currently applied styles, keyed by
  // style index.
  std::map<size_t, boost::signals2::scoped_connection> style_connections_;
};
