#include "filesystem/file_util.h"

#include "base/client_paths.h"
#include "base/path_service.h"

std::filesystem::path GetPublicFilePath(const std::filesystem::path& path) {
  std::filesystem::path public_path;
  base::PathService::Get(client::DIR_PUBLIC, &public_path);
  return public_path / path;
}

std::filesystem::path FullFilePathToPublic(const std::filesystem::path& path) {
  return path.filename();
}
