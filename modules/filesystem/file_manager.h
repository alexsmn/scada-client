#pragma once

#include "base/awaitable.h"
#include "node_service/node_ref.h"

#include <filesystem>

// TODO: Combine with `FileSynchronizer`.
class FileManager {
 public:
  // Owned as `std::unique_ptr<FileManager>` by `FileSystemComponent`, so the
  // concrete implementation must be destroyed through this base.
  virtual ~FileManager() = default;

  // Downloads file from server and saves it to public path. May use cached file
  // if it's already downloaded.
  virtual Awaitable<void> DownloadFileFromServer(
      const std::filesystem::path& path) const = 0;

  // Downloads a known file node and saves it to `path` under public path.
  // The default keeps existing path-based implementations working.
  virtual Awaitable<void> DownloadFileFromServer(
      NodeRef file_node,
      const std::filesystem::path& path) const {
    co_await DownloadFileFromServer(path);
  }
};
