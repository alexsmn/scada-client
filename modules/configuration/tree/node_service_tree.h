#pragma once

#include "node_service/node_ref.h"

// The purpose of this class is to map the viewed node subgraph into a tree
// based on specified filters. It also notifies observers of tree changes.
class NodeServiceTree {
 public:
  virtual ~NodeServiceTree() = default;

  virtual NodeRef GetRoot() const = 0;

  struct ChildRef {
    scada::NodeId reference_type_id;
    bool forward = true;
    NodeRef child_node;
  };

  virtual bool HasChildren(const NodeRef& node) const = 0;

  virtual std::vector<ChildRef> GetChildren(const NodeRef& node) const = 0;

  class Observer {
   public:
    virtual ~Observer() = default;

    virtual void OnNodeDeleted(const scada::NodeId& node_id) = 0;
    virtual void OnNodeChildrenChanged(const scada::NodeId& node_id) = 0;
    virtual void OnNodeModelChanged(const scada::NodeId& node_id) = 0;
    virtual void OnNodeSemanticsChanged(const scada::NodeId& node_id) = 0;
  };

  virtual void SetObserver(Observer* observer) = 0;
};
