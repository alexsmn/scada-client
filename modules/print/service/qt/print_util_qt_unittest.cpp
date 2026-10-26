#include "print/service/print_util.h"

#include "aui/test/app_environment.h"
#include "export/image_export_model.h"

#include <gmock/gmock.h>

#include <QImage>
#include <QPainter>

using namespace testing;

namespace {

// A 200x100 picture painted solid red, so the test can find where it landed.
class RedPicture : public ImageExportModel {
 public:
  QSize GetImageSize() const override { return {200, 100}; }
  void RenderImage(QPainter& painter) override {
    painter.fillRect(QRect{QPoint{}, GetImageSize()}, Qt::red);
  }
};

bool IsRed(const QImage& image, int x, int y) {
  return image.pixelColor(x, y) == QColor{Qt::red};
}

class PrintImagePageTest : public Test {
 protected:
  AppEnvironment app_env_;
};

// Backlog 874: a graph prints as its picture, under the same header a table
// printout carries, scaled to the page width with its aspect ratio kept.
TEST_F(PrintImagePageTest, ScalesThePictureBelowTheHeader) {
  QImage page{1000, 1000, QImage::Format_ARGB32};
  page.fill(Qt::white);
  RedPicture picture;
  {
    QPainter painter{&page};
    PaintImagePage(painter, page.rect(), picture,
                   PrintHeader{.title = u"Graph", .lines = {u"Period"}});
  }

  // The header occupies the top, so the picture starts below it.
  EXPECT_FALSE(IsRed(page, 500, 0));

  // Full width; twice as wide as tall, so 1000x500 starting at some |top|.
  int top = 0;
  while (top < page.height() && !IsRed(page, 500, top))
    ++top;
  ASSERT_LT(top, page.height()) << "the picture was not painted";
  EXPECT_GT(top, 0);
  EXPECT_TRUE(IsRed(page, 1, top + 10));
  EXPECT_TRUE(IsRed(page, 998, top + 10));
  EXPECT_TRUE(IsRed(page, 500, top + 495));
  EXPECT_FALSE(IsRed(page, 500, top + 505));
}

TEST_F(PrintImagePageTest, ATallPictureIsFittedToTheHeightAndCentred) {
  QImage page{1000, 300, QImage::Format_ARGB32};
  page.fill(Qt::white);
  RedPicture picture;
  {
    QPainter painter{&page};
    PaintImagePage(painter, page.rect(), picture, PrintHeader{});
  }

  // 300 high fits a 600-wide picture, centred: 200..800.
  EXPECT_FALSE(IsRed(page, 150, 150));
  EXPECT_TRUE(IsRed(page, 250, 150));
  EXPECT_TRUE(IsRed(page, 750, 150));
  EXPECT_FALSE(IsRed(page, 850, 150));
}

}  // namespace
