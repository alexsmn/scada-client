#include "file_synchronizer.h"

#include "base/files/file_util.h"
#include "base/logger.h"
#include "common/file_system.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "common/scada_node_ids.h"

namespace {

bool IsFileEqual(const std::filesystem::path& path,
                 const scada::ByteString& hash,
                 scada::UInt64 size) {
  if (size != 0 && hash.empty())
    return false;

  std::error_code ec;
  auto actual_size = std::filesystem::file_size(path, ec);
  if (ec)
    return false;

  if (actual_size != size)
    return false;

  auto actual_hash = CalculateFileHash(path);
  return actual_hash == hash;
}

}  // namespace

FileSynchronizer::FileSynchronizer(FileSynchronizerContext&& context)
    : FileSynchronizerContext{std::move(context)} {
  logger_->WriteF(LogSeverity::Normal, "Fetch file tree...");

  const auto& root = node_service_.GetNode(id::FileSystem);
  FetchTree(root, [this, root] {
    logger_->WriteF(LogSeverity::Normal, "Fetch file tree completed");
    AddNodesRecursively(root, root_dir_);
  });
}

void FileSynchronizer::AddNodesRecursively(const NodeRef& root,
                                           const std::filesystem::path& path) {
  for (const auto& child : root.targets(scada::id::Organizes)) {
    const auto& child_path = root_dir_ / child.browse_name().name();
    AddNode(child, child_path);
    AddNodesRecursively(child, child_path);
  }
}

void FileSynchronizer::AddNode(const NodeRef& node,
                               const std::filesystem::path& path) {
  assert(node.fetched());

  if (!IsInstanceOf(node, id::FileType))
    return;

  const auto& hash =
      node[id::FileType_Hash].value().get_or(scada::ByteString{});
  auto size =
      node[id::FileType_Size].value().get_or(static_cast<scada::UInt64>(0));

  if (IsFileEqual(path, hash, size)) {
    logger_->WriteF(LogSeverity::Normal, "File '%s' is actual",
                    path.string().c_str());
    return;
  }

  logger_->WriteF(LogSeverity::Normal, "Download outdated '%s'...",
                  path.string().c_str());

  node.Read(scada::AttributeId::Value, [this, path](scada::DataValue&& value) {
    if (!scada::IsGood(value.status_code)) {
      logger_->WriteF(LogSeverity::Warning, "Download '%s' error: %s",
                      path.string().c_str(),
                      ToString(value.status_code).c_str());
      return;
    }

    auto* data = value.value.get_if<scada::ByteString>();
    if (!data) {
      logger_->WriteF(LogSeverity::Warning,
                      "Wrong downloaded data for file '%s'",
                      path.string().c_str());
      return;
    }

    logger_->WriteF(LogSeverity::Normal, "Download '%s' complete",
                    path.string().c_str());
    base::WriteFile(base::FilePath{path.wstring()}, data->data(), data->size());
  });
}
