#pragma once

#include "base/blinker.h"
#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>
#include <functional>

#include <memory>

namespace Gdiplus {
class Brush;
class Graphics;
class Pen;
class RectF;
}  // namespace Gdiplus

class BlinkerManager;

class ModusStyle2 : private Blinker {
 public:
  explicit ModusStyle2(BlinkerManager& blinker_manager);
  ~ModusStyle2();

  void set_brush(std::unique_ptr<Gdiplus::Brush> brush);
  void set_animation_brush(std::unique_ptr<Gdiplus::Brush> brush);

  void set_pen(std::unique_ptr<Gdiplus::Pen> pen);
  void set_animation_pen(std::unique_ptr<Gdiplus::Pen> pen);

  void Paint(Gdiplus::Graphics& graphics,
             const Gdiplus::RectF& rect,
             bool background);

  using AnimationStepCallback = std::function<void()>;

  // Notifies on every animation (blink) step. Returns an empty connection if
  // the style is not animated. The blink timer starts with the first
  // subscription and stops after the last one disconnects.
  [[nodiscard]] boost::signals2::scoped_connection SubscribeAnimationStep(
      const AnimationStepCallback& callback);

 private:
  bool IsAnimated() const;

  // Blinker
  virtual void OnBlink(bool state) override;

  std::unique_ptr<Gdiplus::Brush> brush_;
  std::unique_ptr<Gdiplus::Brush> animation_brush_;

  std::unique_ptr<Gdiplus::Pen> pen_;
  std::unique_ptr<Gdiplus::Pen> animation_pen_;

  boost::signals2::signal<void()> animation_step_signal_;
};