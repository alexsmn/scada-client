#pragma once

#include "base/awaitable.h"
#include "base/executor.h"
#include "filesystem/file_manager.h"
#include "scada/client.h"
#include "scada/node_id.h"

#include <memory>

struct FileManagerContext {
  // Drives coroutine continuations posted by `AwaitPromise`. The public
  // API still returns `promise<void>`, so the executor lives here rather
  // than on the interface.
  const std::shared_ptr<Executor> executor_;
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
  // Internals are coroutines that `co_await` the promise-returning
  // `scada::client` helpers through `AwaitPromise`. The public entry
  // above spawns `DownloadFileFromServerAsync` on `executor_` and
  // hands back a `promise<void>` so callers do not change.
  Awaitable<void> DownloadFileFromServerAsync(
      std::filesystem::path path) const;

  // `path` is a relative path from public path.
  Awaitable<scada::NodeId> GetFileNodeAsync(
      std::filesystem::path path) const;
};
