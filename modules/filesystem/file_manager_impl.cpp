#include "filesystem/file_manager_impl.h"

#include "filesystem/file_util.h"
#include "model/filesystem_node_ids.h"

#include "base/utf_convert.h"

#include <fstream>
#include <span>

namespace {

scada::StatusOr<scada::NodeId> GetOnlyTargetId(
    std::span<const scada::BrowsePathTarget> targets) {
  if (targets.size() != 1) {
    return scada::StatusCode::Bad;
  }

  return targets[0].target_id.node_id();
}

}  // namespace

Awaitable<void> FileManagerImpl::DownloadFileFromServer(
    const std::filesystem::path& path) const {
  co_await DownloadFileFromServerAsync(path);
}

Awaitable<void> FileManagerImpl::DownloadFileFromServerAsync(
    std::filesystem::path path) const {
  const scada::NodeId file_node_id = co_await GetFileNodeAsync(path);
  // TODO: Check cache.
  const auto file_value =
      co_await scada_client_.node(file_node_id).read_value();
  if (!file_value.ok()) {
    co_return;
  }

  const auto* contents = file_value->value.get_if<scada::ByteString>();
  if (!contents) {
    co_return;
  }

  const auto public_path = GetPublicFilePath(path);
  std::error_code ec;
  std::filesystem::create_directories(public_path.parent_path(), ec);
  std::ofstream ofs{public_path, std::ios::binary};
  ofs.write(contents->data(), contents->size());
  if (!ofs) {
    co_return;
  }
  co_return;
}

Awaitable<scada::NodeId> FileManagerImpl::GetFileNodeAsync(
    std::filesystem::path path) const {
  if (path.empty()) {
    co_return scada::NodeId{};
  }

  scada::RelativePath relative_path;
  for (const auto& c : path) {
    relative_path.emplace_back(scada::RelativePathElement{
        .reference_type_id = scada::id::Organizes,
        .target_name = UtfConvert<char>(c.wstring())});
  }

  auto targets = co_await scada_client_.node(filesystem::id::FileSystem)
                     .translate_browse_path(relative_path);
  if (!targets.ok()) {
    co_return scada::NodeId{};
  }
  auto node_id = GetOnlyTargetId(*targets);
  co_return node_id.ok() ? *node_id : scada::NodeId{};
}
