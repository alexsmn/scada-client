#pragma once

#include "base/any_executor.h"

#include "base/awaitable.h"
#include "base/any_executor_dispatch.h"
#include "filesystem/file_manager.h"
#include "scada/client.h"
#include "scada/node_id.h"

#include <memory>

struct FileManagerContext {
  const AnyExecutor executor_;
  scada::client scada_client_;
};

// TODO: Combine with `FileSynchronizer`.
class FileManagerImpl : private FileManagerContext, public FileManager {
 public:
  explicit FileManagerImpl(FileManagerContext&& context)
      : FileManagerContext{std::move(context)} {}

  // FileManager
  virtual Awaitable<void> DownloadFileFromServer(
      const std::filesystem::path& path) const override;

 private:
  Awaitable<void> DownloadFileFromServerAsync(
      std::filesystem::path path) const;

  // `path` is a relative path from public path.
  Awaitable<scada::NodeId> GetFileNodeAsync(
      std::filesystem::path path) const;
};
