#pragma once

#include "model/scada_node_ids.h"
#include "node_service/node_util.h"

class CreateTree {
 public:
  bool CanCreate(const NodeRef& parent,
                 const NodeRef& component_type_definition) {
    parent.Fetch(NodeFetchStatus::NodeOnly());

    for (const auto& creates : parent.targets(scada::id::Creates)) {
      component_type_definition.Fetch(NodeFetchStatus::NodeOnly());
      if (IsSubtypeOf(component_type_definition, creates.node_id()))
        return true;
    }

    for (auto type_definition = parent.type_definition(); type_definition;
         type_definition = type_definition.supertype()) {
      for (const auto& creates : type_definition.targets(scada::id::Creates)) {
        if (IsSubtypeOf(component_type_definition, creates.node_id()))
          return true;
      }
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
