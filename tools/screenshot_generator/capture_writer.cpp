#include "capture_writer.h"

#include "base/no_destructor.h"

#include <QImage>
#include <QString>

#include <cstdio>
#include <format>
#include <iostream>
#include <system_error>

namespace {

double Seconds(CaptureTimeline::Clock::duration duration) {
  return std::chrono::duration<double>(duration).count();
}

// The run's timeline, started by the first capture written: the generator's
// `main` is the shared gtest runner's, so there is no earlier hook of ours to
// start it from. Times are therefore relative to the first capture, and the
// first line's gap is zero — what the timeline locates is a stall BETWEEN
// captures, which is the shape backlog 898 measured.
CaptureTimeline& Timeline() {
  static scada::base::NoDestructor<CaptureTimeline> timeline{
      CaptureTimeline::Clock::now()};
  return *timeline;
}

// Whether the file at `path` already shows exactly `image`. Both sides are
// normalised to one format first: QImage::operator== treats differing
// formats as different images, and the grab and the decoded PNG routinely
// come back in different ones (RGB32 against RGB888) for the same pixels.
bool AlreadyHolds(const std::filesystem::path& path, const QImage& image) {
  std::error_code error;
  if (!std::filesystem::is_regular_file(path, error))
    return false;

  const QImage existing{QString::fromStdString(path.string())};
  if (existing.isNull() || existing.size() != image.size())
    return false;

  // pHYs is written from these, so a resolution change is a byte change the
  // pixels alone would not show.
  if (existing.dotsPerMeterX() != image.dotsPerMeterX() ||
      existing.dotsPerMeterY() != image.dotsPerMeterY()) {
    return false;
  }

  return existing.convertToFormat(QImage::Format_ARGB32) ==
         image.convertToFormat(QImage::Format_ARGB32);
}

// Does the write, and names what happened for the timeline line.
std::string_view Write(const QImage& image, const std::filesystem::path& path) {
  if (image.isNull())
    return "FAILED: empty image";

  if (AlreadyHolds(path, image)) {
    std::error_code error;
    std::filesystem::last_write_time(
        path, std::filesystem::file_time_type::clock::now(), error);
    return error ? "FAILED: could not touch" : "unchanged";
  }

  return image.save(QString::fromStdString(path.string()), "PNG")
             ? "written"
             : "FAILED: could not write";
}

}  // namespace

std::string CaptureTimeline::Record(std::string_view filename,
                                    std::string_view outcome,
                                    Clock::time_point now) {
  const Clock::time_point since = previous_.value_or(start_);
  previous_ = now;
  return std::format("capture {} ({}) at {:.1f}s, +{:.1f}s", filename, outcome,
                     Seconds(now - start_), Seconds(now - since));
}

bool WriteCapture(const QImage& image, const std::filesystem::path& path) {
  const std::string_view outcome = Write(image, path);
  std::cerr << Timeline().Record(path.filename().string(), outcome,
                                 CaptureTimeline::Clock::now())
            << std::endl;
  return !outcome.starts_with("FAILED");
}
