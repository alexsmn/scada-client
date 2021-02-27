#pragma once

#include "node_service/node_ref.h"

class NodeServiceTree {
 public:
  virtual ~NodeServiceTree() = default;

  virtual NodeRef GetRoot() const = 0;

  struct ChildRef {
    scada::NodeId reference_type_id;
    bool forward = true;
    NodeRef child_node;
  };

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
