#pragma once

#include "base/any_executor.h"

#include "configuration/tree/node_service_tree.h"

#include <boost/signals2/connection.hpp>
#include <memory>

class NodeService;

struct NodeServiceTreeImplContext {
  using ReferenceFilter = std::vector<
      std::pair<scada::NodeId /*reference_type_id*/, bool /*forward*/>>;

  const AnyExecutor executor_;
  NodeService& node_service_;
  const NodeRef root_node_;
  const ReferenceFilter reference_filter_;
  const std::vector<scada::NodeId> type_definition_ids_;
  // Types that cannot be expanded in the tree.
  const std::vector<scada::NodeId> leaf_type_definition_ids_;
};

class NodeServiceTreeImpl : public NodeServiceTree,
                            private NodeServiceTreeImplContext {
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

  void OnModelChanged(const scada::ModelChangeEvent& event);
  void OnNodeSemanticChanged(const scada::NodeId& node_id);

  Observer* observer_ = nullptr;

  boost::signals2::scoped_connection model_changed_connection_;
  boost::signals2::scoped_connection node_semantic_changed_connection_;
};
