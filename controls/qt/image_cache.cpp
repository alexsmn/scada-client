#include "controls/qt/image_cache.h"

#include "base/win/scoped_gdi_object.h"
#include "base/win/scoped_hdc.h"
#include "base/win/scoped_select_object.h"

#include <algorithm>
#include <objidl.h>

using std::max;
using std::min;

#include <gdiplus.h>
#include <gdiplusgraphics.h>
#include <qwinfunctions.h>

namespace {

base::win::ScopedBitmap BuildBitmap(Gdiplus::Image& image) {
  const int width = image.GetWidth();
  const int height = image.GetHeight();

  base::win::ScopedCreateDC dc{::CreateCompatibleDC(NULL)};
  base::win::ScopedBitmap bitmap{
      ::CreateCompatibleBitmap(dc.Get(), width, height)};
  base::win::ScopedSelectObject select_bitmap{dc.Get(), bitmap.get()};

  Gdiplus::Graphics graphics{dc.Get()};
  const auto status = graphics.DrawImage(&image, 0, 0, width, height);
  assert(status == Gdiplus::Ok);

  return bitmap;
}

}  // namespace

// ImageCache

QPixmap ImageCache::Convert(const gfx::Image& image) {
  auto i = map_.find(image);
  if (i != map_.end())
    return i->second;

  auto pixmap = BuildPixmap(image);
  map_.emplace(image, pixmap);
  return pixmap;
}

//  static
QPixmap ImageCache::BuildPixmap(const gfx::Image& image) {
  auto* gdiplus_image = image.native_image();
  if (!gdiplus_image)
    return QPixmap{};

  auto bitmap = BuildBitmap(*gdiplus_image);
  return QtWin::fromHBITMAP(bitmap.get());
}