#include "aui/qt/image_util.h"

#include "aui/test/app_environment.h"

#include <QColor>
#include <QDir>
#include <QImage>
#include <QTemporaryDir>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>

namespace scada::aui {
namespace {

// A Lucide-shaped glyph: 24 grid, stroke 2, round caps, currentColor. A
// diagonal so every quadrant of the box has some coverage.
constexpr char kGlyph[] =
    R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24")"
    R"( viewBox="0 0 24 24" fill="none" stroke="currentColor")"
    R"( stroke-width="2" stroke-linecap="round" stroke-linejoin="round">)"
    R"(<path d="M3 3 L21 21"/></svg>)";

// The bounding box of the glyph's drawn pixels, expressed as a fraction of the
// icon's extent so renders at different device pixel ratios are comparable.
// Returned as left/top/right/bottom, all in [0, 1].
std::array<double, 4> NormalizedInkBounds(const QIcon& icon,
                                          int size,
                                          qreal device_pixel_ratio) {
  const QImage image =
      icon.pixmap(QSize{size, size}, device_pixel_ratio).toImage();
  int left = image.width();
  int top = image.height();
  int right = -1;
  int bottom = -1;
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      if (qAlpha(image.pixel(x, y)) <= 128)
        continue;
      left = std::min(left, x);
      right = std::max(right, x);
      top = std::min(top, y);
      bottom = std::max(bottom, y);
    }
  }
  EXPECT_GE(right, 0) << "nothing was drawn";
  const double w = image.width();
  const double h = image.height();
  return {left / w, top / h, (right + 1) / w, (bottom + 1) / h};
}

class TintedGlyphTest : public testing::Test {
 protected:
  // Returns the path of a temp .svg holding `contents`.
  std::string WriteGlyph(const char* name, const char* contents) {
    const QString path = dir_.filePath(QString::fromLatin1(name));
    QFile file{path};
    EXPECT_TRUE(file.open(QIODevice::WriteOnly));
    file.write(contents);
    file.close();
    return path.toStdString();
  }

  AppEnvironment app_env_;
  QTemporaryDir dir_;
};

// The files carry stroke="currentColor", which Qt resolves to black. The point
// of tinting is that one asset serves dark, light and high-contrast — so the
// rendered pixels must be the requested colour, not the file's.
TEST_F(TintedGlyphTest, RendersTheGlyphInTheRequestedTint) {
  const std::string path = WriteGlyph("glyph.svg", kGlyph);

  const QIcon icon = LoadTintedGlyph(path, 16, QColor{255, 0, 0});
  ASSERT_FALSE(icon.isNull());

  const QImage image = icon.pixmap(16, 16).toImage();
  int tinted = 0;
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      const QColor pixel = image.pixelColor(x, y);
      if (pixel.alpha() > 128) {
        ++tinted;
        EXPECT_GT(pixel.red(), 200) << "at " << x << "," << y;
        EXPECT_LT(pixel.green(), 60);
        EXPECT_LT(pixel.blue(), 60);
      }
    }
  }
  EXPECT_GT(tinted, 0) << "nothing was drawn";
}

// The same asset in a second colour: what makes it a theme-following glyph
// rather than a baked one.
TEST_F(TintedGlyphTest, TheSameAssetServesASecondTheme) {
  const std::string path = WriteGlyph("glyph.svg", kGlyph);

  const QImage dark =
      LoadTintedGlyph(path, 16, QColor{230, 230, 230}).pixmap(16, 16).toImage();
  const QImage light =
      LoadTintedGlyph(path, 16, QColor{30, 30, 30}).pixmap(16, 16).toImage();

  ASSERT_EQ(dark.size(), light.size());
  EXPECT_NE(dark, light);
}

// Rendered at the device pixel ratio, so a 16 px row glyph is crisp on a HiDPI
// display rather than upscaled from 16 physical pixels the way the retired
// bitmap strips were.
TEST_F(TintedGlyphTest, RendersAtTheDevicePixelRatio) {
  const std::string path = WriteGlyph("glyph.svg", kGlyph);

  const QIcon icon =
      LoadTintedGlyph(path, 16, QColor{255, 255, 255}, /*dpr=*/2.0);

  ASSERT_FALSE(icon.isNull());
  // The icon holds 32 physical pixels for a 16 logical-pixel glyph. What Qt
  // then hands back from pixmap() is its own scaling policy and not worth
  // pinning; what matters here is that the detail was rendered, not upscaled.
  ASSERT_FALSE(icon.availableSizes().isEmpty());
  EXPECT_EQ(icon.availableSizes().first(), QSize(32, 32));
}

// A higher device pixel ratio buys resolution, never a bigger glyph: the drawn
// artwork must occupy the same fraction of the icon at every ratio.
//
// Regression test. QPainter takes *logical* coordinates and applies the
// pixmap's device pixel ratio itself, so a destination rect given in device
// pixels scaled the glyph by the ratio twice — at dpr 2 it was drawn at double
// size and clipped by the pixmap, which showed up as toolbar icons that were a
// zoomed crop overflowing their button. Only the ratio-1 case was ever right,
// so a check on the icon's pixel dimensions alone (above) cannot see this.
TEST_F(TintedGlyphTest, ADeviceRatioChangesResolutionNotGeometry) {
  const std::string path = WriteGlyph("glyph.svg", kGlyph);

  const std::array<double, 4> at_1x = NormalizedInkBounds(
      LoadTintedGlyph(path, 16, QColor{255, 255, 255}, /*dpr=*/1.0), 16, 1.0);
  const std::array<double, 4> at_2x = NormalizedInkBounds(
      LoadTintedGlyph(path, 16, QColor{255, 255, 255}, /*dpr=*/2.0), 16, 2.0);

  // The glyph's own margin: the stroke spans 2..22 of a 24 viewBox, so it must
  // stay clear of the icon's edge rather than run into it.
  EXPECT_LT(at_2x[2], 0.97) << "the glyph is clipped against the icon's edge";
  EXPECT_LT(at_2x[3], 0.97) << "the glyph is clipped against the icon's edge";

  // One logical pixel of tolerance, for the rounding the two rasters differ by.
  constexpr double kTolerance = 1.0 / 16;
  for (size_t i = 0; i < at_1x.size(); ++i)
    EXPECT_NEAR(at_2x[i], at_1x[i], kTolerance) << "edge " << i;
}

// A missing or unparseable resource yields a null icon, which consumers
// already treat as "no icon" — not a placeholder box, and not a crash.
TEST_F(TintedGlyphTest, MissingOrInvalidResourceYieldsANullIcon) {
  EXPECT_TRUE(LoadTintedGlyph(":/does/not/exist.svg", 16, Qt::white).isNull());
  const std::string broken = WriteGlyph("broken.svg", "not an svg at all");
  EXPECT_TRUE(LoadTintedGlyph(broken, 16, Qt::white).isNull());
}

// Index order is the models' "tile index" contract, inherited from the sliced
// bitmap strip: entry N must stay entry N.
TEST_F(TintedGlyphTest, GlyphSetPreservesIndexOrder) {
  const std::string good = WriteGlyph("glyph.svg", kGlyph);
  const std::array<std::string_view, 3> paths{good, ":/missing.svg", good};

  const std::vector<QIcon> icons =
      LoadTintedGlyphs(paths, 16, QColor{255, 255, 255});

  ASSERT_EQ(icons.size(), 3u);
  EXPECT_FALSE(icons[0].isNull());
  EXPECT_TRUE(icons[1].isNull()) << "a gap must stay a gap, not shift the rest";
  EXPECT_FALSE(icons[2].isNull());
}

}  // namespace
}  // namespace scada::aui
