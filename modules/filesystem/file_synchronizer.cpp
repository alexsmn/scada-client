#include "filesystem/file_synchronizer.h"

#include "base/awaitable.h"
#include "base/boost_log.h"
#include "filesystem/filesystem_util.h"
#include "model/filesystem_node_ids.h"
#include "net/net_executor_adapter.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "scada/event.h"

#include <fstream>

#if defined(UI_QT)
#include <QUrl>
#endif

namespace {

#if defined(UI_QT)
std::string DecodeUri(std::string_view str) {
  auto data = QByteArray::fromPercentEncoding(
      QByteArray::fromRawData(str.data(), str.size()));
  return data.toStdString();
}
#else
std::string DecodeUri(std::string_view str) {
  return std::string{str};
}
#endif

std::filesystem::file_time_type ToFileTime(scada::Time time) {
  if (scada::IsNull(time))
    return std::filesystem::file_time_type{};
  else if (time == scada::kMinTime)
    return std::filesystem::file_time_type::min();
  else if (time == scada::kMaxTime)
    return std::filesystem::file_time_type::max();
  else {
    auto delta = time - scada::Time{};
    return std::filesystem::file_time_type{
        std::chrono::microseconds(delta.count())};
  }
}

Awaitable<void> DownloadFileNodeAsync(
    AnyExecutor executor,
    std::shared_ptr<BoostLogger> logger,
    NodeRef node,
    std::filesystem::path path,
    std::filesystem::file_time_type last_update_time) {
  auto data_value = co_await node.scada_node().read(scada::AttributeId::Value);
  if (!data_value.ok()) {
    LOG_WARNING(*logger) << std::format("Download '{}' error: {}",
                   path.string(), ToString(data_value.status()));
    co_return;
  }

  auto* data = data_value->value.get_if<scada::ByteString>();
  if (!data) {
    LOG_WARNING(*logger) << std::format("Wrong downloaded data for file '{}'",
                   path.string());
    co_return;
  }

  LOG_INFO(*logger) << std::format("Download '{}' complete", path.string());

  std::ofstream{path, std::ios::binary}.write(data->data(), data->size());

  std::error_code ec;
  std::filesystem::last_write_time(path, last_update_time, ec);

  co_return;
}

}  // namespace

FileSynchronizer::FileSynchronizer(FileSynchronizerContext&& context)
    : FileSynchronizerContext{std::move(context)} {
  LOG_INFO(*logger_) << std::format("Fetch file tree...");

  connections_.push_back(node_service_.SubscribeModelChanged(
      [this](const scada::ModelChangeEvent& event) { OnModelChanged(event); }));
  connections_.push_back(node_service_.SubscribeNodeSemanticChanged(
      [this](const scada::NodeId& node_id) {
        OnNodeSemanticChanged(node_id);
      }));

  const auto& root = node_service_.GetNode(scada::filesystem::id::FileSystem);
  CoSpawn(executor_, [this, root]() -> Awaitable<void> {
    co_await FetchTree(root);
    if (root.status()) {
      LOG_INFO(*logger_) << std::format("Fetch file tree completed");
      ProcessNodesRecursively(root);
    } else {
      LOG_INFO(*logger_) << std::format("File-system is disabled");
    }
  });
}

FileSynchronizer::~FileSynchronizer() = default;

void FileSynchronizer::ProcessNodesRecursively(NodeRef root) {
  for (const auto& child : root.targets(scada::id::Organizes)) {
    if (ProcessNode(child))
      ProcessNodesRecursively(child);
  }
}

bool FileSynchronizer::ProcessNode(NodeRef node) {
  // Synchronizer receives updates for all items.
  // assert(node.fetched());

  if (IsInstanceOf(node, scada::filesystem::id::FileType))
    return ProcessFileNode(node);
  else if (IsInstanceOf(node, scada::filesystem::id::FileDirectoryType))
    return ProcessFileDirectoryNode(node);
  else
    return false;
}

bool FileSynchronizer::ProcessFileDirectoryNode(NodeRef node) {
  const auto& path = root_dir_ / GetFilePath(node);

  std::error_code ec;
  if (std::filesystem::is_directory(path, ec)) {
    LOG_INFO(*logger_) << std::format("Directory '{}' is actual",
                    path.string());
    return true;
  }

  LOG_INFO(*logger_) << std::format("Create directory '{}'", path.string());

  if (!std::filesystem::create_directories(path, ec)) {
    LOG_INFO(*logger_) << std::format("Create directory '{}' error: {}",
                    path.string(), ec.message());
    return false;
  }

  return true;
}

bool FileSynchronizer::ProcessFileNode(NodeRef node) {
  const auto& path = root_dir_ / GetFilePath(node);

  auto last_update_time = ToFileTime(
      node[scada::filesystem::id::FileType_LastUpdateTime].value().get_or(
          scada::Time{}));

  std::error_code ec;
  auto actual_last_update_time = std::filesystem::last_write_time(path, ec);
  if (actual_last_update_time == last_update_time) {
    LOG_INFO(*logger_) << std::format("File '{}' is actual", path.string());
    return true;
  }

  LOG_INFO(*logger_) << std::format("Download outdated '{}'", path.string());

  CoSpawn(executor_, [executor = executor_, logger = logger_, node, path,
                      last_update_time] {
    return DownloadFileNodeAsync(executor, logger, node, path,
                                 last_update_time);
  });

  return true;
}

void FileSynchronizer::OnModelChanged(const scada::ModelChangeEvent& event) {
  if (event.verb & (scada::ModelChangeEvent::NodeAdded |
                    scada::ModelChangeEvent::ReferenceAdded)) {
    ProcessNodesRecursively(node_service_.GetNode(event.node_id));
  }
}

void FileSynchronizer::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  ProcessNode(node_service_.GetNode(node_id));
}

void FileSynchronizer::FetchFileNode(NodeRef node,
                                     const FetchCallback& callback) {
  node_queue_.emplace(node);
  if (callback)
    callbacks_[node].emplace_back(callback);
}
