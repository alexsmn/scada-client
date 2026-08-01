#include "aui/qt/image_util.h"

#include "aui/test/app_environment.h"

#include <QColor>
#include <QDir>
#include <QImage>
#include <QTemporaryDir>
#include <gtest/gtest.h>

#include <array>
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

  const QImage dark = LoadTintedGlyph(path, 16, QColor{230, 230, 230})
                          .pixmap(16, 16)
                          .toImage();
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
