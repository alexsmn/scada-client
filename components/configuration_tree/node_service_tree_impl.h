#pragma once

#include "components/configuration_tree/node_service_tree.h"
#include "node_service/node_observer.h"

#include <memory>

class Executor;
class NodeService;

struct NodeServiceTreeImplContext {
  using ReferenceFilter = std::vector<
      std::pair<scada::NodeId /*reference_type_id*/, bool /*forward*/>>;

  const std::shared_ptr<Executor> executor_;
  NodeService& node_service_;
  const NodeRef root_node_;
  const ReferenceFilter reference_filter_;
  const std::vector<scada::NodeId> type_definition_ids_;
};

class NodeServiceTreeImpl : public NodeServiceTree,
                            private NodeServiceTreeImplContext,
                            private NodeRefObserver {
 public:
  explicit NodeServiceTreeImpl(NodeServiceTreeImplContext&& context);
  ~NodeServiceTreeImpl();

  // NodeServiceTree
  virtual NodeRef GetRoot() const override;
  virtual bool HasChildren(const NodeRef& node) const override;
  virtual std::vector<ChildRef> GetChildren(const NodeRef& node) const override;
  virtual void SetObserver(Observer* observer) override;

 private:
  bool IsMatchingNode(const NodeRef& node) const;

  // NodeRefObserver
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

  Observer* observer_ = nullptr;
};
