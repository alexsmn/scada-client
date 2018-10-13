#pragma once

#include "common/node_observer.h"

#include <filesystem>
#include <memory>

class Logger;
class NodeRef;
class NodeService;

struct FileSynchronizerContext {
  const std::shared_ptr<const Logger> logger_;
  NodeService& node_service_;
  const std::filesystem::path root_dir_;
};

class FileSynchronizer : private FileSynchronizerContext,
                         private NodeRefObserver {
 public:
  explicit FileSynchronizer(FileSynchronizerContext&& context);
  ~FileSynchronizer();

 private:
  void ProcessNodesRecursively(const NodeRef& root,
                               const std::filesystem::path& path);
  bool ProcessNode(const NodeRef& node, const std::filesystem::path& path);
  bool ProcessFileDirectoryNode(const NodeRef& node,
                                const std::filesystem::path& path);
  bool ProcessFileNode(const NodeRef& node, const std::filesystem::path& path);

  // NodeRefObserver
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;
};
