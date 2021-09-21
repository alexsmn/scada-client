#pragma once

#include "gfx/image/image.h"

#include <QPixmap>
#include <unordered_map>

class ImageCache {
 public:
  QPixmap Convert(const gfx::Image& image);

 private:
  static QPixmap BuildPixmap(const gfx::Image& image);

  std::unordered_map<gfx::Image, QPixmap> map_;
};
