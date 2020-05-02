#include "file_synchronizer.h"

#include "base/files/file_util.h"
#include "base/logger.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "model/filesystem_node_ids.h"
#include "model/scada_node_ids.h"

namespace {

std::filesystem::file_time_type ToFileTime(scada::DateTime time) {
  if (time.is_null())
    return std::filesystem::file_time_type{};
  else if (time.is_min())
    return std::filesystem::file_time_type::min();
  else if (time.is_max())
    return std::filesystem::file_time_type::max();
  else {
    auto delta = time - scada::DateTime::UnixEpoch();
    return std::filesystem::file_time_type{
        std::chrono::microseconds(delta.InMicroseconds())};
  }
}

bool IsFileNode(const NodeRef& node) {
  return IsSubtypeOf(node, filesystem::id::FileDirectoryType) ||
         IsSubtypeOf(node, filesystem::id::FileType);
}

std::optional<std::filesystem::path> GetNodeRelativePath(const NodeRef& node) {
  if (!IsFileNode(node))
    return std::nullopt;

  std::filesystem::path path;
  for (auto n = node; IsFileNode(n); n = n.parent())
    path = std::filesystem::path{n.browse_name().name()} / path;
  return path;
}

}  // namespace

FileSynchronizer::FileSynchronizer(FileSynchronizerContext&& context)
    : FileSynchronizerContext{std::move(context)} {
  logger_->WriteF(LogSeverity::Normal, "Fetch file tree...");

  node_service_.Subscribe(*this);

  const auto& root = node_service_.GetNode(filesystem::id::FileSystem);
  FetchTree(root, [this, root] {
    logger_->WriteF(LogSeverity::Normal, "Fetch file tree completed");
    ProcessNodesRecursively(root, root_dir_);
  });
}

FileSynchronizer::~FileSynchronizer() {
  node_service_.Unsubscribe(*this);
}

void FileSynchronizer::ProcessNodesRecursively(
    const NodeRef& root,
    const std::filesystem::path& path) {
  for (const auto& child : root.targets(scada::id::Organizes)) {
    const auto& child_path = root_dir_ / child.browse_name().name();
    if (ProcessNode(child, child_path))
      ProcessNodesRecursively(child, child_path);
  }
}

bool FileSynchronizer::ProcessNode(const NodeRef& node,
                                   const std::filesystem::path& path) {
  assert(node.fetched());

  if (IsInstanceOf(node, filesystem::id::FileType))
    return ProcessFileNode(node, path);
  else if (IsInstanceOf(node, filesystem::id::FileDirectoryType))
    return ProcessFileDirectoryNode(node, path);
  else
    return false;
}

bool FileSynchronizer::ProcessFileDirectoryNode(
    const NodeRef& node,
    const std::filesystem::path& path) {
  std::error_code ec;
  if (std::filesystem::is_directory(path, ec)) {
    logger_->WriteF(LogSeverity::Normal, "Directory '%s' is actual",
                    path.string().c_str());
    return true;
  }

  logger_->WriteF(LogSeverity::Normal, "Create directory '%s'",
                  path.string().c_str());

  if (!std::filesystem::create_directories(path, ec)) {
    logger_->WriteF(LogSeverity::Normal, "Create directory '%s' error: %s",
                    path.string().c_str(), ec.message().c_str());
    return false;
  }

  return true;
}

bool FileSynchronizer::ProcessFileNode(const NodeRef& node,
                                       const std::filesystem::path& path) {
  auto last_update_time =
      ToFileTime(node[filesystem::id::FileType_LastUpdateTime].value().get_or(
          scada::DateTime{}));
  auto size = node[filesystem::id::FileType_Size].value().get_or(
      static_cast<scada::UInt64>(0));

  std::error_code ec;
  auto actual_last_update_time = std::filesystem::last_write_time(path, ec);
  if (actual_last_update_time == last_update_time) {
    logger_->WriteF(LogSeverity::Normal, "File '%s' is actual",
                    path.string().c_str());
    return true;
  }

  logger_->WriteF(LogSeverity::Normal, "Download outdated '%s'",
                  path.string().c_str());

  node.Read(scada::AttributeId::Value, [this, path, last_update_time](
                                           scada::DataValue&& value) {
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

    std::error_code ec;
    std::filesystem::last_write_time(path, last_update_time, ec);
  });

  return true;
}

void FileSynchronizer::OnModelChanged(const scada::ModelChangeEvent& event) {
  if (event.verb & (scada::ModelChangeEvent::NodeAdded |
                    scada::ModelChangeEvent::ReferenceAdded)) {
    auto node = node_service_.GetNode(event.node_id);
    if (auto path = GetNodeRelativePath(node))
      ProcessNodesRecursively(node, root_dir_ / *path);
  }
}

void FileSynchronizer::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  auto node = node_service_.GetNode(node_id);
  if (auto path = GetNodeRelativePath(node))
    ProcessNode(node, root_dir_ / *path);
}
