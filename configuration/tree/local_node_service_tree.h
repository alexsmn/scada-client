#pragma once

#include "configuration/configuration_module.h"
#include "configuration/tree/node_service_tree.h"
#include "scada/node_id.h"

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace boost::json {
class value;
}

class NodeService;

// In-memory NodeServiceTree backed by a static parent→children map. Answers
// `GetChildren` by resolving each child id through a companion `NodeService`,
// so the tree structure and the node attributes can be populated
// independently.
//
// Intended for tests, demos, and screenshot tooling that need a SCADA back-end
// driven by static data rather than a real server.
class LocalNodeServiceTree : public NodeServiceTree {
 public:
  // Shared state that a LocalNodeServiceTree consults. One instance can be
  // shared across multiple tree objects (e.g. across factory invocations).
  struct SharedData {
    // parent node id → children node ids
    std::map<scada::NodeId, std::vector<scada::NodeId>> children;

    // Populates the `children` map from a screenshot-style JSON document.
    // Tree keys are "<ns>.<id>" or bare "<id>" (implies ns=1); values are
    // arrays of numeric child ids (namespace 1).
    void LoadFromJson(const boost::json::value& root);
  };

  LocalNodeServiceTree(NodeService& node_service,
                       NodeRef root_node,
                       std::shared_ptr<const SharedData> data);
  ~LocalNodeServiceTree() override;

  // Returns a factory compatible with
  // `ClientApplicationContext::node_service_tree_factory_` / configuration
  // module. Each invocation produces a new tree bound to `node_service`,
  // uses the context's `root_node_`, and shares `data`.
  static NodeServiceTreeFactory MakeFactory(
      NodeService& node_service,
      std::shared_ptr<const SharedData> data);

  // NodeServiceTree
  NodeRef GetRoot() const override;
  bool HasChildren(const NodeRef& node) const override;
  std::vector<ChildRef> GetChildren(const NodeRef& node) const override;
  void SetObserver(Observer* /*observer*/) override {}

 private:
  NodeService& node_service_;
  NodeRef root_node_;
  std::shared_ptr<const SharedData> data_;
};
