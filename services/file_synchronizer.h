#pragma once

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

class FileSynchronizer : private FileSynchronizerContext {
 public:
  explicit FileSynchronizer(FileSynchronizerContext&& context);

 private:
  void AddNodesRecursively(const NodeRef& root,
                           const std::filesystem::path& path);
  void AddNode(const NodeRef& node, const std::filesystem::path& path);
};
