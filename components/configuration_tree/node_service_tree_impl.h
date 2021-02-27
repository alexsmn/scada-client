#pragma once

#include "components/configuration_tree/node_service_tree.h"

class NodeService;

struct NodeServiceTreeImplContext {
  using ReferenceFilter = std::vector<
      std::pair<scada::NodeId /*reference_type_id*/, bool /*forward*/>>;

  NodeService& node_service_;
  const NodeRef root_node_;
  const ReferenceFilter reference_filter_;
  const std::vector<scada::NodeId> type_definition_ids_;
};

class NodeServiceTreeImpl : public NodeServiceTree,
                            private NodeServiceTreeImplContext {
 public:
  explicit NodeServiceTreeImpl(NodeServiceTreeImplContext&& context);

  // NodeServiceTree
  virtual NodeRef GetRoot() const override;
  virtual std::vector<ChildRef> GetChildren(const NodeRef& node) const override;

 private:
  bool IsMatchingNode(const NodeRef& node) const;
};
