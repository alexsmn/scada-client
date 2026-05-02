#include "modus/libmodus/modus_style_library2.h"

#include "modus/libmodus/modus_style2.h"
#include "libmodus/gfx/gdip.h"

ModusStyleLibrary2::ModusStyleLibrary2(BlinkerManager& blinker_manager) {
  {
    auto style = std::make_unique<ModusStyle2>(blinker_manager);
    style->set_pen(std::unique_ptr<Gdiplus::Pen>(
        new Gdiplus::Pen(static_cast<Gdiplus::ARGB>(Gdiplus::Color::Red))));
    style->set_brush(std::unique_ptr<Gdiplus::Brush>(
        new Gdiplus::HatchBrush(Gdiplus::HatchStyleForwardDiagonal,
                                static_cast<Gdiplus::ARGB>(Gdiplus::Color::Red),
                                Gdiplus::Color(0, 0, 0, 0))));
    set_style(StyleId::INVAL, std::move(style));
  }

  {
    auto style = std::make_unique<ModusStyle2>(blinker_manager);
    style->set_pen(std::unique_ptr<Gdiplus::Pen>(
        new Gdiplus::Pen(static_cast<Gdiplus::ARGB>(Gdiplus::Color::Orange))));
    style->set_brush(std::unique_ptr<Gdiplus::Brush>(new Gdiplus::HatchBrush(
        Gdiplus::HatchStyleForwardDiagonal,
        static_cast<Gdiplus::ARGB>(Gdiplus::Color::Orange),
        Gdiplus::Color(0, 0, 0, 0))));
    set_style(StyleId::INACT, std::move(style));
  }

  {
    auto style = std::make_unique<ModusStyle2>(blinker_manager);
    style->set_brush(std::unique_ptr<Gdiplus::Brush>(new Gdiplus::SolidBrush(
        static_cast<Gdiplus::ARGB>(Gdiplus::Color::Orange))));
    set_style(StyleId::BADQ, std::move(style));
  }

  {
    auto style = std::make_unique<ModusStyle2>(blinker_manager);
    style->set_pen(std::unique_ptr<Gdiplus::Pen>(new Gdiplus::Pen(
        static_cast<Gdiplus::ARGB>(Gdiplus::Color::Orange), 2.0F)));
    style->set_animation_pen(std::unique_ptr<Gdiplus::Pen>(new Gdiplus::Pen(
        static_cast<Gdiplus::ARGB>(Gdiplus::Color::Lime), 2.0F)));
    set_style(StyleId::ALERT, std::move(style));
  }
}

ModusStyleLibrary2::~ModusStyleLibrary2() {}

ModusStyle2* ModusStyleLibrary2::GetStyle(StyleId id) {
  auto index = static_cast<size_t>(id);
  return index < styles_.size() ? styles_[index].get() : nullptr;
}

void ModusStyleLibrary2::set_style(StyleId id,
                                   std::unique_ptr<ModusStyle2> style) {
  auto index = static_cast<size_t>(id);
  if (index >= styles_.size())
    styles_.resize(index + 1);
  styles_[index] = std::move(style);
}
