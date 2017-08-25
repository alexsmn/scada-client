#pragma once

#include <memory>

#include "base/observer_list.h"
#include "base/blinker.h"

namespace Gdiplus {
class Brush;
class Graphics;
class Pen;
class RectF;
}

class ModusStyle2 : private Blinker {
 public:
  ModusStyle2();
  ~ModusStyle2();

  void set_brush(std::unique_ptr<Gdiplus::Brush> brush);
  void set_animation_brush(std::unique_ptr<Gdiplus::Brush> brush);

  void set_pen(std::unique_ptr<Gdiplus::Pen> pen);
  void set_animation_pen(std::unique_ptr<Gdiplus::Pen> pen);

  void Paint(Gdiplus::Graphics& graphics, const Gdiplus::RectF& rect,
             bool background);

  class AnimationObserver {
   public:
    virtual void OnAnimationStep() = 0;
  };

  void AddAnimationObserver(AnimationObserver& observer);
  void RemoveAnimationObserver(AnimationObserver& observer);

 private:
  bool IsAnimated() const;

  // Blinker
	virtual void OnBlink(bool state) override;

  std::unique_ptr<Gdiplus::Brush> brush_;
  std::unique_ptr<Gdiplus::Brush> animation_brush_;

  std::unique_ptr<Gdiplus::Pen> pen_;
  std::unique_ptr<Gdiplus::Pen> animation_pen_;

  base::ObserverList<AnimationObserver> animation_observers_;
};