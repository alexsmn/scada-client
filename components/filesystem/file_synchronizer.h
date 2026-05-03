#pragma once

#include "base/any_executor.h"

#include "node_service/node_observer.h"
#include "node_service/node_ref.h"

#include <filesystem>
#include <map>
#include <memory>
#include <queue>

class Logger;
class NodeService;

struct FileSynchronizerContext {
  const AnyExecutor executor_;
  const std::shared_ptr<const Logger> logger_;
  NodeService& node_service_;
  const std::filesystem::path root_dir_;
};

class FileSynchronizer : private FileSynchronizerContext,
                         private NodeRefObserver {
 public:
  explicit FileSynchronizer(FileSynchronizerContext&& context);
  ~FileSynchronizer();

  using FetchCallback = std::function<void(bool ok)>;
  void FetchFileNode(NodeRef node, const FetchCallback& callback);

 private:
  void ProcessNodesRecursively(NodeRef root);
  bool ProcessNode(NodeRef node);
  bool ProcessFileDirectoryNode(NodeRef node);
  bool ProcessFileNode(NodeRef node);

  // NodeRefObserver
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

  std::queue<NodeRef> node_queue_;
  std::map<NodeRef, std::vector<FetchCallback>> callbacks_;
};
