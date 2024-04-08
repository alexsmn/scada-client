#pragma once

#include "base/promise.h"

#include <filesystem>

// TODO: Combine with `FileSynchronizer`.
class FileManager {
 public:
  // Downloads file from server and saves it to public path. May use cached file
  // if it's already downloaded.
  virtual promise<void> DownloadFileFromServer(
      const std::filesystem::path& path) const = 0;
};
