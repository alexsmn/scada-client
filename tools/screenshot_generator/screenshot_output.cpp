#include "screenshot_output.h"

#include <QByteArray>

namespace {

std::filesystem::path g_output_dir;
bool g_output_dir_resolved = false;

}  // namespace

std::filesystem::path GetOutputDir() {
  if (g_output_dir_resolved)
    return g_output_dir;

  // qgetenv is the safe CRT wrapper; avoids MSVC's std::getenv deprecation.
  const QByteArray env = qgetenv("SCREENSHOT_OUT_DIR");
  if (!env.isEmpty())
    g_output_dir = std::filesystem::path{env.toStdString()};

  if (g_output_dir.empty())
    g_output_dir = std::filesystem::path{__FILE__}.parent_path() /
                   "../../docs";

  g_output_dir_resolved = true;
  return g_output_dir.lexically_normal();
}
