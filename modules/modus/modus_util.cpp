#include "modus/modus_util.h"

#include "filesystem/file_util.h"

#include <boost/algorithm/string/predicate.hpp>

bool IsModusFilePath(const std::filesystem::path& path) {
  auto ext = path.extension().string();
  return boost::iequals(ext, ".sde") || boost::iequals(ext, ".xsde");
}

std::optional<std::filesystem::path> MakeModusFilePath(
    const std::filesystem::path& hyperlink_path,
    const std::filesystem::path& current_display_path) {
  // An absolute hyperlink names a location on disk, so it has to come back
  // through the public root to be addressable at all.
  if (hyperlink_path.is_absolute()) {
    const std::filesystem::path relative =
        FullFilePathToPublic(hyperlink_path.lexically_normal());
    if (relative.empty() || *relative.begin() == "..")
      return std::nullopt;
    return relative;
  }

  // `current_display_path` is already public-relative, so resolving against
  // its directory keeps the result public-relative — return it as such rather
  // than routing it through `FullFilePathToPublic`, whose job is to strip an
  // absolute public root and which therefore cannot see this one.
  //
  // Normalising *after* joining is what collapses `..`, and the check below is
  // what makes it safe: a hyperlink like `../../etc/passwd.sde` normalises to
  // a path that escapes the public directory, and escaping it is exactly what
  // must not be allowed. Before task 483 the whole result was replaced by its
  // filename, which discarded the directory and quietly opened whichever
  // same-named file the public root held.
  const std::filesystem::path resolved =
      (current_display_path.parent_path() / hyperlink_path).lexically_normal();

  if (resolved.empty() || resolved.is_absolute() || *resolved.begin() == "..")
    return std::nullopt;

  return resolved;
}
