#include "filesystem/file_util.h"

#include "base/client_paths.h"
#include "base/file_path_util.h"
#include "base/path_service.h"

std::filesystem::path GetPublicFilePath(const std::filesystem::path& path) {
  base::FilePath public_path;
  base::PathService::Get(client::DIR_PUBLIC, &public_path);
  return AsFilesystemPath(public_path) / path;
}

std::filesystem::path FullFilePathToPublic(const std::filesystem::path& path) {
  return path.filename();
}
