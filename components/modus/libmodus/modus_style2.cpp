#include "components/modus/libmodus/modus_style2.h"

#include "libmodus/gfx/gdip.h"

ModusStyle2::ModusStyle2() {}

ModusStyle2::~ModusStyle2() {}

void ModusStyle2::set_brush(std::unique_ptr<Gdiplus::Brush> brush) {
  brush_ = std::move(brush);
}

void ModusStyle2::set_animation_brush(std::unique_ptr<Gdiplus::Brush> brush) {
  animation_brush_ = std::move(brush);
}

void ModusStyle2::set_pen(std::unique_ptr<Gdiplus::Pen> pen) {
  pen_ = std::move(pen);
}

void ModusStyle2::set_animation_pen(std::unique_ptr<Gdiplus::Pen> pen) {
  animation_pen_ = std::move(pen);
}

void ModusStyle2::Paint(Gdiplus::Graphics& graphics,
                        const Gdiplus::RectF& rect,
                        bool background) {
  bool animation_state = Blinker::GetState();

  if (brush_) {
    auto& brush =
        (animation_brush_ && animation_state) ? animation_brush_ : brush_;
    bool is_background_brush = brush->GetType() != Gdiplus::BrushTypeHatchFill;
    if (background == is_background_brush)
      graphics.FillRectangle(brush.get(), rect);
  }

  if (!background && pen_) {
    auto& pen = (animation_pen_ && animation_state) ? animation_pen_ : pen_;
    Gdiplus::Matrix m;
    graphics.GetTransform(&m);
    m.Invert();
    pen->SetTransform(&m);
    graphics.DrawRectangle(pen.get(), rect);
  }
}

void ModusStyle2::AddAnimationObserver(AnimationObserver& observer) {
  if (!IsAnimated())
    return;

  bool start = !animation_observers_.might_have_observers();

  animation_observers_.AddObserver(&observer);

  if (start)
    Blinker::Start();
}

void ModusStyle2::RemoveAnimationObserver(AnimationObserver& observer) {
  if (!IsAnimated())
    return;

  animation_observers_.RemoveObserver(&observer);

  if (!animation_observers_.might_have_observers())
    Blinker::Stop();
}

bool ModusStyle2::IsAnimated() const {
  return animation_brush_ || animation_pen_;
}

void ModusStyle2::OnBlink(bool state) {
  assert(IsAnimated());
  FOR_EACH_OBSERVER(AnimationObserver, animation_observers_, OnAnimationStep());
}
