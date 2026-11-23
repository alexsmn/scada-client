#include "aui/qt/status_glyph.h"

#include "aui/test/app_environment.h"

#include <QApplication>
#include <QFont>
#include <QImage>
#include <gtest/gtest.h>

namespace scada::aui {
namespace {

constexpr int kDiameter = 20;

// Alpha of the glyph at a point given as fractions of its diameter.
int AlphaAt(const QImage& image, double fx, double fy) {
  return qAlpha(image.pixel(static_cast<int>(fx * kDiameter),
                            static_cast<int>(fy * kDiameter)));
}

class StatusGlyphTest : public testing::Test {
 protected:
  // Every band in one colour, so only the shape can tell them apart.
  QImage Draw(Quality quality) {
    return DrawStatusGlyph(quality, QColor{Qt::black}, kDiameter, 1.0)
        .toImage()
        .convertToFormat(QImage::Format_ARGB32);
  }

  AppEnvironment app_env_;
};

// Good is a filled disc: solid right through the centre and on both sides.
TEST_F(StatusGlyphTest, GoodIsAFilledDisc) {
  const QImage image = Draw(Quality::kGood);
  EXPECT_GT(AlphaAt(image, 0.5, 0.5), 200);
  EXPECT_GT(AlphaAt(image, 0.3, 0.5), 200);
  EXPECT_GT(AlphaAt(image, 0.75, 0.5), 200);
}

// Uncertain is half filled: the left of the interior is solid, the right empty.
TEST_F(StatusGlyphTest, UncertainIsHalfFilled) {
  const QImage image = Draw(Quality::kUncertain);
  EXPECT_GT(AlphaAt(image, 0.3, 0.5), 200);
  EXPECT_LT(AlphaAt(image, 0.75, 0.5), 60);
  // The outline closes the right half, so it still reads as one disc.
  EXPECT_GT(AlphaAt(image, 0.97, 0.5), 60);
}

// Bad is a ring with «?» inside: the outline is drawn, the interior beside
// the mark is empty, and something (the mark) sits inside it.
TEST_F(StatusGlyphTest, BadIsARingWithAQuestionMark) {
  const QImage image = Draw(Quality::kBad);
  EXPECT_GT(AlphaAt(image, 0.5, 0.03), 60);   // ring, top
  EXPECT_LT(AlphaAt(image, 0.2, 0.5), 60);    // interior, left of the mark
  EXPECT_LT(AlphaAt(image, 0.8, 0.5), 60);    // interior, right of the mark
  int mark = 0;
  for (int y = 5; y < kDiameter - 5; ++y) {
    for (int x = 7; x < kDiameter - 7; ++x)
      mark += qAlpha(image.pixel(x, y)) > 128 ? 1 : 0;
  }
  EXPECT_GT(mark, 5);
}

// The glyph keeps its logical size at any device pixel ratio.
TEST_F(StatusGlyphTest, HonoursTheDevicePixelRatio) {
  const QPixmap pixmap =
      DrawStatusGlyph(Quality::kBad, QColor{Qt::red}, kDiameter, 2.0);
  EXPECT_EQ(pixmap.width(), 2 * kDiameter);
  EXPECT_EQ(pixmap.devicePixelRatio(), 2.0);
}

// The size follows the font, so a larger UI font gets a larger glyph, within
// the 9 px floor that keeps «?» legible.
TEST_F(StatusGlyphTest, DiameterFollowsTheFont) {
  QFont small = QApplication::font();
  small.setPixelSize(6);
  QFont large = QApplication::font();
  large.setPixelSize(18);
  EXPECT_GE(StatusGlyphDiameter(small), 9);
  EXPECT_GT(StatusGlyphDiameter(large), StatusGlyphDiameter(small));
}

}  // namespace
}  // namespace scada::aui
