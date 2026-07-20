#include "modules/table/qt/sparkline_delegate.h"

#include "aui/test/app_environment.h"

#include <QImage>
#include <QPainter>
#include <QStandardItemModel>
#include <QStyleOptionViewItem>
#include <gtest/gtest.h>

#include <vector>

namespace {

// Renders one cell through the delegate into an image.
QImage PaintCell(const std::vector<double>& series) {
  QImage image{120, 24, QImage::Format_ARGB32};
  image.fill(Qt::black);

  QStandardItemModel model{1, 1};
  SparklineDelegate delegate{[&series](int row) { return series; }, nullptr};

  QStyleOptionViewItem option;
  option.rect = QRect{0, 0, image.width(), image.height()};

  QPainter painter{&image};
  delegate.paint(&painter, option, model.index(0, 0));
  return image;
}

class SparklineDelegateTest : public testing::Test {
 protected:
  AppEnvironment app_env_;
};

// A varying series paints a visible polyline; an empty series leaves the cell
// as the plain background.
TEST_F(SparklineDelegateTest, PaintsAPolylineForAVaryingSeries) {
  const QImage empty = PaintCell({});
  const QImage sparkline = PaintCell({1.0, 4.0, 2.0, 5.0, 3.0});
  EXPECT_NE(sparkline, empty);
}

// A single sample has nothing to connect: identical to the empty rendering.
TEST_F(SparklineDelegateTest, SingleSamplePaintsNothing) {
  EXPECT_EQ(PaintCell({4.2}), PaintCell({}));
}

}  // namespace
