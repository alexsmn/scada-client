#include "filesystem/file_synchronizer.h"

#include "base/awaitable.h"
#include "base/awaitable_promise.h"
#include "base/logger.h"
#include "filesystem/filesystem_util.h"
#include "model/filesystem_node_ids.h"
#include "net/net_executor_adapter.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "scada/event.h"
#include "scada/status_exception.h"

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

Awaitable<void> DownloadFileNodeAsync(std::shared_ptr<Executor> executor,
                                      std::shared_ptr<const Logger> logger,
                                      NodeRef node,
                                      std::filesystem::path path,
                                      std::filesystem::file_time_type
                                          last_update_time) {
  try {
    auto data_value = co_await AwaitPromise(
        NetExecutorAdapter{executor},
        node.scada_node().read(scada::AttributeId::Value));

    auto* data = data_value.value.get_if<scada::ByteString>();
    if (!data) {
      logger->WriteF(LogSeverity::Warning,
                     "Wrong downloaded data for file '{}'", path.string());
      co_return;
    }

    logger->WriteF(LogSeverity::Normal, "Download '{}' complete",
                   path.string());

    std::ofstream{path, std::ios::binary}.write(data->data(), data->size());

    std::error_code ec;
    std::filesystem::last_write_time(path, last_update_time, ec);

  } catch (...) {
    logger->WriteF(LogSeverity::Warning, "Download '{}' error: {}",
                   path.string(),
                   ToString(scada::GetExceptionStatus(std::current_exception())));
  }

  co_return;
}

}  // namespace

FileSynchronizer::FileSynchronizer(FileSynchronizerContext&& context)
    : FileSynchronizerContext{std::move(context)} {
  logger_->WriteF(LogSeverity::Normal, "Fetch file tree...");

  node_service_.Subscribe(*this);

  const auto& root = node_service_.GetNode(filesystem::id::FileSystem);
  FetchTree(root, [this, root] {
    if (root.status()) {
      logger_->WriteF(LogSeverity::Normal, "Fetch file tree completed");
      ProcessNodesRecursively(root);
    } else {
      logger_->WriteF(LogSeverity::Normal, "File-system is disabled");
    }
  });
}

FileSynchronizer::~FileSynchronizer() {
  node_service_.Unsubscribe(*this);
}

void FileSynchronizer::ProcessNodesRecursively(NodeRef root) {
  for (const auto& child : root.targets(scada::id::Organizes)) {
    if (ProcessNode(child))
      ProcessNodesRecursively(child);
  }
}

bool FileSynchronizer::ProcessNode(NodeRef node) {
  // Synchronizer receives updates for all items.
  // assert(node.fetched());

  if (IsInstanceOf(node, filesystem::id::FileType))
    return ProcessFileNode(node);
  else if (IsInstanceOf(node, filesystem::id::FileDirectoryType))
    return ProcessFileDirectoryNode(node);
  else
    return false;
}

bool FileSynchronizer::ProcessFileDirectoryNode(NodeRef node) {
  const auto& path = root_dir_ / GetFilePath(node);

  std::error_code ec;
  if (std::filesystem::is_directory(path, ec)) {
    logger_->WriteF(LogSeverity::Normal, "Directory '{}' is actual",
                    path.string());
    return true;
  }

  logger_->WriteF(LogSeverity::Normal, "Create directory '{}'",
                  path.string());

  if (!std::filesystem::create_directories(path, ec)) {
    logger_->WriteF(LogSeverity::Normal, "Create directory '{}' error: {}",
                    path.string(), ec.message());
    return false;
  }

  return true;
}

bool FileSynchronizer::ProcessFileNode(NodeRef node) {
  const auto& path = root_dir_ / GetFilePath(node);

  auto last_update_time =
      ToFileTime(node[filesystem::id::FileType_LastUpdateTime].value().get_or(
          scada::DateTime{}));

  std::error_code ec;
  auto actual_last_update_time = std::filesystem::last_write_time(path, ec);
  if (actual_last_update_time == last_update_time) {
    logger_->WriteF(LogSeverity::Normal, "File '{}' is actual", path.string());
    return true;
  }

  logger_->WriteF(LogSeverity::Normal, "Download outdated '{}'", path.string());

  CoSpawn(executor_,
          [executor = executor_, logger = logger_, node, path,
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
