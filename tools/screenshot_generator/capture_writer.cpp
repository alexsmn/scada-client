#include "capture_writer.h"

#include "base/no_destructor.h"

#include <QImage>
#include <QString>

#include <cstdio>
#include <format>
#include <iostream>
#include <ostream>
#include <system_error>

#if defined(__APPLE__)
#include <time.h>
#endif

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
      CaptureTimeline::Now()};
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

CaptureTimeline::Instant CaptureTimeline::Now() {
  Instant now{.run = Clock::now()};
#if defined(__APPLE__)
  now.awake = std::chrono::nanoseconds{clock_gettime_nsec_np(CLOCK_UPTIME_RAW)};
#endif
  return now;
}

std::string CaptureTimeline::Record(std::string_view filename,
                                    std::string_view outcome,
                                    Instant now) {
  const Instant since = previous_.value_or(start_);
  previous_ = now;
  const Clock::duration gap = now.run - since.run;
  std::string line =
      std::format("capture {} ({}) at {:.1f}s, +{:.1f}s", filename, outcome,
                  Seconds(now.run - start_.run), Seconds(gap));
  if (now.awake && since.awake) {
    const Clock::duration asleep =
        gap -
        std::chrono::duration_cast<Clock::duration>(*now.awake - *since.awake);
    if (asleep >= std::chrono::seconds{1})
      line += std::format(", {:.1f}s of it asleep", Seconds(asleep));
  }
  return line;
}

bool WriteCapture(const QImage& image, const std::filesystem::path& path) {
  const std::string_view outcome = Write(image, path);
  std::cerr << Timeline().Record(path.filename().string(), outcome,
                                 CaptureTimeline::Now())
            << std::endl;
  return !outcome.starts_with("FAILED");
}
