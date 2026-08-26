#include "filesystem/file_util.h"

#include "base/client_paths.h"
#include "base/path_service.h"

#include <stdexcept>

std::filesystem::path GetPublicFilePath(const std::filesystem::path& path) {
  std::filesystem::path public_path;
  if (!scada::base::PathService::Get(client::DIR_PUBLIC, &public_path) ||
      public_path.empty()) {
    throw std::runtime_error{"Cannot resolve client public directory"};
  }
  return public_path / path;
}

std::filesystem::path FullFilePathToPublic(const std::filesystem::path& path) {
  // The inverse of `GetPublicFilePath`: strip the public root, keep whatever
  // subdirectory the file sits in. This used to be `path.filename()`, which is
  // right only for a file directly in the public root and silently wrong for
  // any other — a hyperlink to `../other/scheme.sde` resolved to whichever
  // `scheme.sde` the public root happened to hold (task 483).
  std::filesystem::path public_path;
  if (!scada::base::PathService::Get(client::DIR_PUBLIC, &public_path) ||
      public_path.empty()) {
    return path.filename();
  }

  // `lexically_relative` returns an empty path when the two share no prefix,
  // which is the "outside the public directory" case; and a result that starts
  // with `..` means the same thing. Neither is addressable as a public path,
  // so fall back to the bare filename rather than handing back a traversal.
  const std::filesystem::path relative =
      path.lexically_normal().lexically_relative(
          public_path.lexically_normal());
  if (relative.empty() || *relative.begin() == "..")
    return path.filename();

  return relative;
}
