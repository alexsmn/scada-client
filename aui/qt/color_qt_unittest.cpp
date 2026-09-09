#include "aui/qt/color_qt.h"

#include <QColor>
#include <gtest/gtest.h>

namespace scada::aui {
namespace {

// `Color::operator==` used to be defaulted over the wrapped `QColor`, whose own
// equality is colour-spec sensitive, while `operator<=>` compared the rgba —
// so two colours could be neither less, greater, nor equal. Both now compare
// the rgba.
TEST(ColorQtTest, EqualityIgnoresTheQColorSpec) {
  const Color rgb{QColor{255, 0, 0}};
  const Color hsv{QColor{255, 0, 0}.toHsv()};
  ASSERT_NE(rgb.qcolor().spec(), hsv.qcolor().spec());
  ASSERT_NE(rgb.qcolor(), hsv.qcolor());

  EXPECT_EQ(rgb, hsv);
  EXPECT_EQ(rgb <=> hsv, std::strong_ordering::equal);
}

TEST(ColorQtTest, EqualityStillSeesADifferentChannel) {
  const Color red{Rgba{255, 0, 0, 255}};
  const Color translucent_red{Rgba{255, 0, 0, 128}};
  EXPECT_NE(red, translucent_red);
}

}  // namespace
}  // namespace scada::aui
