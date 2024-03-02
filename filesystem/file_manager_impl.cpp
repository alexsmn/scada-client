#include "filesystem/file_manager_impl.h"

#include "base/file_path_util.h"
#include "base/files/file_util.h"
#include "filesystem/file_util.h"
#include "model/filesystem_node_ids.h"

#include <boost/locale/encoding_utf.hpp>
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

scada::status_promise<void> FileManagerImpl::DownloadFileFromServer(
    const std::filesystem::path& path) const {
  return GetFileNode(path)
      .then([this](const scada::NodeId& file_node_id) {
        // TODO: Check cache.
        return scada_client_.node(file_node_id).read_value();
      })
      .then([](const scada::DataValue& file_value) {
        const auto* contents = file_value.value.get_if<scada::ByteString>();
        if (!contents) {
          throw scada::status_exception{scada::StatusCode::Bad};
        }
        return *contents;
      })
      .then([path](const scada::ByteString& contents) {
        auto public_path = GetPublicFilePath(path);
        std::error_code ec;
        std::filesystem::create_directories(public_path.parent_path(), ec);
        int written = base::WriteFile(AsFilePath(public_path), contents.data(),
                                      contents.size());
        if (written != static_cast<int>(contents.size())) {
          throw scada::status_exception{scada::StatusCode::Bad};
        }
      });
}

scada::status_promise<scada::NodeId> FileManagerImpl::GetFileNode(
    const std::filesystem::path& path) const {
  if (path.empty()) {
    return scada::MakeRejectedStatusPromise<scada::NodeId>(
        scada::StatusCode::Bad);
  }

  scada::RelativePath relative_path;
  for (const auto& c : path) {
    relative_path.emplace_back(scada::RelativePathElement{
        .reference_type_id = scada::id::Organizes,
        .target_name = boost::locale::conv::utf_to_utf<char>(c.wstring())});
  }

  return scada_client_.node(filesystem::id::FileSystem)
      .translate_browse_path(relative_path)
      .then(&GetOnlyTargetId);
}
