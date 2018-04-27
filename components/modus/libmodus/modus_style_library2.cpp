#include "components/modus/modus_style_library2.h"

#include "base/memory/singleton.h"
#include "components/modus/modus_style2.h"
#include "libmodus/gfx/gdip.h"

ModusStyleLibrary2::ModusStyleLibrary2() {
  {
    std::unique_ptr<ModusStyle2> style(new ModusStyle2);
    style->set_pen(
        std::unique_ptr<Gdiplus::Pen>(new Gdiplus::Pen(Gdiplus::Color::Red)));
    style->set_brush(std::unique_ptr<Gdiplus::Brush>(new Gdiplus::HatchBrush(
        Gdiplus::HatchStyleForwardDiagonal, Gdiplus::Color::Red,
        Gdiplus::Color(0, 0, 0, 0))));
    set_style(StyleId::INVAL, std::move(style));
  }

  {
    std::unique_ptr<ModusStyle2> style(new ModusStyle2);
    style->set_pen(std::unique_ptr<Gdiplus::Pen>(
        new Gdiplus::Pen(Gdiplus::Color::Orange)));
    style->set_brush(std::unique_ptr<Gdiplus::Brush>(new Gdiplus::HatchBrush(
        Gdiplus::HatchStyleForwardDiagonal, Gdiplus::Color::Orange,
        Gdiplus::Color(0, 0, 0, 0))));
    set_style(StyleId::INACT, std::move(style));
  }

  {
    std::unique_ptr<ModusStyle2> style(new ModusStyle2);
    style->set_brush(std::unique_ptr<Gdiplus::Brush>(
        new Gdiplus::SolidBrush(Gdiplus::Color::Orange)));
    set_style(StyleId::BADQ, std::move(style));
  }

  {
    std::unique_ptr<ModusStyle2> style(new ModusStyle2);
    style->set_pen(std::unique_ptr<Gdiplus::Pen>(
        new Gdiplus::Pen(Gdiplus::Color::Orange, 2.0F)));
    style->set_animation_pen(std::unique_ptr<Gdiplus::Pen>(
        new Gdiplus::Pen(Gdiplus::Color::Lime, 2.0F)));
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
