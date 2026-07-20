#include "modus/libmodus/modus_style2.h"

#include "base/check.h"
#include "libmodus/gfx/gdip.h"

ModusStyle2::ModusStyle2(BlinkerManager& blinker_manager)
    : Blinker{blinker_manager} {}

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

boost::signals2::scoped_connection ModusStyle2::SubscribeAnimationStep(
    const AnimationStepCallback& callback) {
  if (!IsAnimated())
    return {};

  bool start = animation_step_signal_.num_slots() == 0;

  auto connection = animation_step_signal_.connect(callback);

  if (start)
    Blinker::Start();

  return boost::signals2::scoped_connection{connection};
}

bool ModusStyle2::IsAnimated() const {
  return animation_brush_ || animation_pen_;
}

void ModusStyle2::OnBlink(bool state) {
  scada::base::Check(IsAnimated());

  // The blink timer keeps running until the step after the last subscriber
  // disconnected; disconnects are only detected here.
  if (animation_step_signal_.num_slots() == 0) {
    Blinker::Stop();
    return;
  }

  animation_step_signal_();
}
