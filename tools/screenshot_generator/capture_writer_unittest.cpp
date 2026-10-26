#include "capture_writer.h"

#include <QColor>
#include <QImage>
#include <QString>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include <fstream>
#include <iterator>
#include <string>

namespace {

using namespace std::chrono_literals;

// A small image with enough structure that the encoder has row filters to
// choose between, so two compression levels genuinely produce different bytes.
QImage MakeImage(QColor accent) {
  QImage image{64, 32, QImage::Format_RGB32};
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x)
      image.setPixel(x, y, qRgb((x * 7) % 256, (y * 13) % 256, (x ^ y) % 256));
  }
  image.setPixelColor(5, 5, accent);
  return image;
}

std::string ReadBytes(const std::filesystem::path& path) {
  std::ifstream in{path, std::ios::binary};
  return {std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
}

class CaptureWriterTest : public testing::Test {
 protected:
  std::filesystem::path PathFor(const char* name) const {
    return std::filesystem::path{dir_.path().toStdString()} / name;
  }

  QTemporaryDir dir_;
};

TEST_F(CaptureWriterTest, WritesANewFile) {
  const auto path = PathFor("new.png");
  ASSERT_TRUE(WriteCapture(MakeImage(Qt::red), path));

  const QImage written{QString::fromStdString(path.string())};
  EXPECT_EQ(written.convertToFormat(QImage::Format_ARGB32),
            MakeImage(Qt::red).convertToFormat(QImage::Format_ARGB32));
}

// The 832 case: the file on disk shows exactly the image being written, but
// was encoded differently — here by another compression level, in the gallery
// by another generator binary. Rewriting it would be a diff with no UI change
// behind it, so the bytes must survive.
TEST_F(CaptureWriterTest, KeepsTheBytesWhenOnlyTheEncodingDiffers) {
  const auto path = PathFor("same.png");
  ASSERT_TRUE(MakeImage(Qt::red).save(QString::fromStdString(path.string()),
                                      "PNG", /*quality=*/0));
  const std::string before = ReadBytes(path);

  // Guard the premise: a default-level encode really is different bytes, so
  // the assertion below is about WriteCapture and not about a lucky encoder.
  const auto probe = PathFor("probe.png");
  ASSERT_TRUE(
      MakeImage(Qt::red).save(QString::fromStdString(probe.string()), "PNG"));
  ASSERT_NE(ReadBytes(probe), before);

  ASSERT_TRUE(WriteCapture(MakeImage(Qt::red), path));
  EXPECT_EQ(ReadBytes(path), before);
}

// `capture_provenance.py --produced-since` finds a pass's renders by mtime, so
// a kept file must still look freshly rendered.
TEST_F(CaptureWriterTest, TouchesAKeptFile) {
  const auto path = PathFor("touched.png");
  ASSERT_TRUE(WriteCapture(MakeImage(Qt::red), path));
  const auto old_time = std::filesystem::file_time_type::clock::now() - 1h;
  std::filesystem::last_write_time(path, old_time);

  ASSERT_TRUE(WriteCapture(MakeImage(Qt::red), path));
  EXPECT_GT(std::filesystem::last_write_time(path), old_time + 30min);
}

TEST_F(CaptureWriterTest, RewritesWhenAPixelChanged) {
  const auto path = PathFor("changed.png");
  ASSERT_TRUE(WriteCapture(MakeImage(Qt::red), path));
  ASSERT_TRUE(WriteCapture(MakeImage(Qt::blue), path));

  const QImage written{QString::fromStdString(path.string())};
  EXPECT_EQ(written.pixelColor(5, 5), QColor{Qt::blue});
}

TEST_F(CaptureWriterTest, RewritesWhenTheSizeChanged) {
  const auto path = PathFor("resized.png");
  ASSERT_TRUE(WriteCapture(MakeImage(Qt::red), path));
  ASSERT_TRUE(WriteCapture(MakeImage(Qt::red).copy(0, 0, 32, 32), path));

  EXPECT_EQ(QImage{QString::fromStdString(path.string())}.size(),
            QSize(32, 32));
}

// A file that is not an image is not "the same image"; it is replaced.
TEST_F(CaptureWriterTest, ReplacesAFileThatIsNotAnImage) {
  const auto path = PathFor("garbage.png");
  std::ofstream{path} << "not a png";
  ASSERT_TRUE(WriteCapture(MakeImage(Qt::red), path));
  EXPECT_FALSE(QImage{QString::fromStdString(path.string())}.isNull());
}

TEST_F(CaptureWriterTest, AnEmptyImageIsAFailure) {
  EXPECT_FALSE(WriteCapture(QImage{}, PathFor("empty.png")));
  EXPECT_FALSE(std::filesystem::exists(PathFor("empty.png")));
}

// The 898 instrument: each line carries the time into the run and the gap
// since the previous capture, so a stall is the line after a large `+`.
TEST(CaptureTimelineTest, ReportsRunTimeAndTheGapSinceThePreviousCapture) {
  const CaptureTimeline::Clock::time_point start{};
  CaptureTimeline timeline{start};

  EXPECT_EQ(timeline.Record("a.png", "written", start + 2s),
            "capture a.png (written) at 2.0s, +2.0s");
  EXPECT_EQ(timeline.Record("b.png", "unchanged", start + 2500ms),
            "capture b.png (unchanged) at 2.5s, +0.5s");
  EXPECT_EQ(timeline.Record("c.png", "written", start + 870s),
            "capture c.png (written) at 870.0s, +867.5s");
}

}  // namespace
