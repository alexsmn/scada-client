#pragma once

#include "filesystem/file_manager.h"
#include "scada/client.h"
#include "scada/node_id.h"

struct FileManagerContext {
  scada::client scada_client_;
};

// TODO: Combine with `FileSynchronizer`.
class FileManagerImpl : private FileManagerContext, public FileManager {
 public:
  explicit FileManagerImpl(FileManagerContext&& context)
      : FileManagerContext{std::move(context)} {}

  // FileManager
  virtual promise<void> DownloadFileFromServer(
      const std::filesystem::path& path) const override;

 private:
  // `path` is a relative path from public path.
  promise<scada::NodeId> GetFileNode(
      const std::filesystem::path& path) const;
};
