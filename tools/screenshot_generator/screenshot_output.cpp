#include "screenshot_output.h"

#include <QByteArray>

#include <stdexcept>

namespace {

std::filesystem::path g_output_dir;
bool g_output_dir_resolved = false;

}  // namespace

std::filesystem::path GetOutputDir() {
  if (g_output_dir_resolved)
    return g_output_dir;

  // qgetenv is the safe CRT wrapper; avoids MSVC's std::getenv deprecation.
  const QByteArray env = qgetenv("SCREENSHOT_OUT_DIR");
  if (env.isEmpty()) {
    throw std::runtime_error{
        "SCREENSHOT_OUT_DIR must be provided for screenshot_generator"};
  }
  g_output_dir = std::filesystem::path{env.toStdString()};

  g_output_dir_resolved = true;
  return g_output_dir.lexically_normal();
}
