#pragma once

#include "base/any_executor.h"
#include "base/boost_log.h"

#include "node_service/node_ref.h"

#include <boost/signals2/connection.hpp>
#include <filesystem>
#include <map>
#include <memory>
#include <queue>

class NodeService;

struct FileSynchronizerContext {
  const AnyExecutor executor_;
  const std::shared_ptr<BoostLogger> logger_;
  NodeService& node_service_;
  const std::filesystem::path root_dir_;
};

class FileSynchronizer : private FileSynchronizerContext {
 public:
  explicit FileSynchronizer(FileSynchronizerContext&& context);
  ~FileSynchronizer();

 private:
  void ProcessNodesRecursively(NodeRef root);
  bool ProcessNode(NodeRef node);
  bool ProcessFileDirectoryNode(NodeRef node);
  bool ProcessFileNode(NodeRef node);

  void OnModelChanged(const scada::ModelChangeEvent& event);
  void OnNodeSemanticChanged(const scada::NodeId& node_id);

  std::queue<NodeRef> node_queue_;

  std::vector<boost::signals2::scoped_connection> connections_;
};
