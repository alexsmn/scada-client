#pragma once

#include "model/scada_node_ids.h"
#include "node_service/node_util.h"

class CreateTree {
 public:
  bool CanCreate(const NodeRef& parent,
                 const NodeRef& component_type_definition) {
    parent.StartFetch(NodeFetchStatus::NodeOnly);

    // A component may be created under `parent` if `parent`'s type chain
    // declares a placeholder for the component's type (or a supertype).
    for (const auto& creatable : GetCreatableChildTypes(parent)) {
      component_type_definition.StartFetch(NodeFetchStatus::NodeOnly);
      if (IsSubtypeOf(component_type_definition, creatable.node_id()))
        return true;
    }

    return false;
  }

  NodeRef GetCreateParentNode(const NodeRef& suggested_parent,
                              const NodeRef& root,
                              const NodeRef& component_type) {
    if (!component_type)
      return nullptr;

    for (const auto& parent : {suggested_parent, root}) {
      if (parent && CanCreate(parent, component_type))
        return parent;
    }

    return nullptr;
  }
};
