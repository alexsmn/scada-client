#pragma once

#include "filesystem/file_manager.h"
#include "scada/node_id.h"

namespace scada {
class client;
}

struct FileManagerContext {
  scada::client& scada_client_;
};

// TODO: Combine with `FileSynchronizer`.
class FileManagerImpl : private FileManagerContext, public FileManager {
 public:
  explicit FileManagerImpl(FileManagerContext&& context)
      : FileManagerContext{std::move(context)} {}

  // FileManager
  virtual scada::status_promise<void> DownloadFileFromServer(
      const std::filesystem::path& path) const override;

 private:
  // `path` is a relative path from public path.
  scada::status_promise<scada::NodeId> GetFileNode(
      const std::filesystem::path& path) const;
};
