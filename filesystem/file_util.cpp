#include "filesystem/file_util.h"

#include "base/client_paths.h"
#include "base/path_service.h"

#include <stdexcept>

std::filesystem::path GetPublicFilePath(const std::filesystem::path& path) {
  std::filesystem::path public_path;
  if (!base::PathService::Get(client::DIR_PUBLIC, &public_path) ||
      public_path.empty()) {
    throw std::runtime_error{"Cannot resolve client public directory"};
  }
  return public_path / path;
}

std::filesystem::path FullFilePathToPublic(const std::filesystem::path& path) {
  return path.filename();
}
