#include "filesystem/file_manager.h"

#include "base/file_path_util.h"
#include "base/files/file_util.h"
#include "filesystem/file_util.h"
#include "model/filesystem_node_ids.h"
#include "scada/attribute_service_promises.h"
#include "scada/view_service_promises.h"

#include <boost/locale/encoding_utf.hpp>

scada::status_promise<void> FileManager::DownloadFileFromServer(
    const std::filesystem::path& path) const {
  return GetFileNode(path)
      .then([this](const scada::NodeId& node_id) {
        // TODO: Check cache.
        return scada::Read(attribute_service_,
                           scada::ServiceContext::default_instance(),
                           {node_id});
      })
      .then([](const scada::DataValue& file_value) {
        const auto* contents = file_value.value.get_if<scada::ByteString>();
        if (!contents) {
          throw scada::status_exception{scada::StatusCode::Bad};
        }
        return *contents;
      })
      .then([path](const scada::ByteString& contents) {
        int written = base::WriteFile(AsFilePath(GetPublicFilePath(path)),
                                      contents.data(), contents.size());
        if (written != static_cast<int>(contents.size())) {
          throw scada::status_exception{scada::StatusCode::Bad};
        }
      });
}

scada::status_promise<scada::NodeId> FileManager::GetFileNode(
    const std::filesystem::path& path) const {
  scada::RelativePath relative_path;
  for (const auto& c : path) {
    relative_path.emplace_back(scada::RelativePathElement{
        .reference_type_id = scada::id::Organizes,
        .target_name = boost::locale::conv::utf_to_utf<char>(c.wstring())});
  }

  return TranslateBrowsePathToOneTarget(
      view_service_, {filesystem::id::FileSystem, std::move(relative_path)});
}
