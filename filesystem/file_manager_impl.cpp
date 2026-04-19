#include "filesystem/file_manager_impl.h"

#include "base/awaitable_promise.h"
#include "filesystem/file_util.h"
#include "model/filesystem_node_ids.h"
#include "scada/status_exception.h"
#include "scada/status_promise.h"

#include "base/utf_convert.h"

#include <fstream>
#include <span>

namespace {

scada::NodeId GetOnlyTargetId(
    std::span<const scada::BrowsePathTarget> targets) {
  if (targets.size() != 1) {
    throw scada::status_exception{scada::StatusCode::Bad};
  }

  return targets[0].target_id.node_id();
}

}  // namespace

promise<void> FileManagerImpl::DownloadFileFromServer(
    const std::filesystem::path& path) const {
  return ToPromise(NetExecutorAdapter{executor_},
                   DownloadFileFromServerAsync(path));
}

Awaitable<void> FileManagerImpl::DownloadFileFromServerAsync(
    std::filesystem::path path) const {
  const scada::NodeId file_node_id = co_await GetFileNodeAsync(path);
  // TODO: Check cache.
  const scada::DataValue file_value = co_await AwaitPromise(
      NetExecutorAdapter{executor_},
      scada_client_.node(file_node_id).read_value());

  const auto* contents = file_value.value.get_if<scada::ByteString>();
  if (!contents) {
    throw scada::status_exception{scada::StatusCode::Bad};
  }

  const auto public_path = GetPublicFilePath(path);
  std::error_code ec;
  std::filesystem::create_directories(public_path.parent_path(), ec);
  std::ofstream ofs{public_path, std::ios::binary};
  ofs.write(contents->data(), contents->size());
  if (!ofs) {
    throw scada::status_exception{scada::StatusCode::Bad};
  }
  co_return;
}

Awaitable<scada::NodeId> FileManagerImpl::GetFileNodeAsync(
    std::filesystem::path path) const {
  if (path.empty()) {
    throw scada::status_exception{scada::StatusCode::Bad};
  }

  scada::RelativePath relative_path;
  for (const auto& c : path) {
    relative_path.emplace_back(scada::RelativePathElement{
        .reference_type_id = scada::id::Organizes,
        .target_name = UtfConvert<char>(c.wstring())});
  }

  auto targets = co_await AwaitPromise(
      NetExecutorAdapter{executor_},
      scada_client_.node(filesystem::id::FileSystem)
          .translate_browse_path(relative_path));
  co_return GetOnlyTargetId(targets);
}
