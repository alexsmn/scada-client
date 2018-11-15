#include "node_serialization.h"

#include "common/node_state.h"
#include "common/node_util.h"
#include "core/standard_node_ids.h"
#include "node_serialization.h"
#include "remote/protocol_utils.h"

void NodeToData(const NodeRef& source,
                scada::NodeState& target,
                bool recursive) {
  assert(source.node_class().has_value());

  target.node_id = source.node_id();
  target.node_class = source.node_class().value();

  const auto& parent_ref =
      source.inverse_reference(scada::id::HierarchicalReferences);
  target.parent_id = parent_ref.target.node_id();
  target.reference_type_id = parent_ref.reference_type.node_id();

  if (auto type_definition = source.type_definition())
    target.type_definition_id = type_definition.node_id();

  target.attributes.browse_name = source.browse_name();
  target.attributes.display_name = source.display_name();

  if (auto data_type = source.data_type())
    target.attributes.data_type = data_type.node_id();

  for (auto type_definition = source.type_definition(); type_definition;
       type_definition = type_definition.supertype()) {
    for (const auto& prop_type :
         type_definition.targets(scada::id::HasProperty)) {
      auto value = source[prop_type.node_id()].value();
      if (!value.is_null())
        target.properties.emplace_back(prop_type.node_id(), std::move(value));
    }
  }

  for (auto& ref : source.references()) {
    assert(ref.forward);
    // Skip type definitions.
    if (!IsSubtypeOf(ref.reference_type, scada::id::HasTypeDefinition) &&
        !IsSubtypeOf(ref.reference_type, scada::id::HasProperty)) {
      target.references.push_back(
          {ref.reference_type.node_id(), ref.forward, ref.target.node_id()});
    }
  }

  if (recursive) {
    for (const auto& child : source.targets(scada::id::Organizes))
      NodeToData(child, target.children.emplace_back(), true);
  }
}

scada::ReferenceDescription FromProto(const protocol::NodeReference& source) {
  return {
      FromProto(source.reference_type_id()),
      source.forward(),
      FromProto(source.node_id()),
  };
}

void ToProto(const scada::ReferenceDescription& source,
             protocol::NodeReference& target) {
  ToProto(source.reference_type_id, *target.mutable_reference_type_id());
  target.set_forward(source.forward);
  ToProto(source.node_id, *target.mutable_node_id());
}

scada::NodeProperty FromProto(const protocol::NodeProperty& source) {
  return {FromProto(source.declaration_id()), FromProto(source.value())};
}

void ToProto(const scada::NodeProperty& source,
             protocol::NodeProperty& target) {
  ToProto(source.first, *target.mutable_declaration_id());
  ToProto(source.second, *target.mutable_value());
}

void ToProto(const scada::NodeState& source, protocol::Node& target) {
  if (!source.parent_id.is_null())
    ToProto(source.parent_id, *target.mutable_parent_id());

  if (!source.reference_type_id.is_null())
    ToProto(source.reference_type_id, *target.mutable_reference_type_id());

  ToProto(source.node_id, *target.mutable_node_id());
  target.set_node_class(ToProto(source.node_class));

  if (!source.type_definition_id.is_null()) {
    assert(!scada::IsTypeDefinition(source.node_class));
    ToProto(source.type_definition_id, *target.mutable_type_definition_id());
  }

  ToProto(source.attributes, *target.mutable_attributes());
  ToProto(source.properties, *target.mutable_property());
  ToProto(source.references, *target.mutable_reference());

  if (!source.children.empty())
    ToProto(source.children, *target.mutable_children());
}

scada::NodeState FromProto(const protocol::Node& source) {
  return {
      FromProto(source.node_id()),
      FromProto(source.node_class()),
      FromProto(source.type_definition_id()),
      FromProto(source.parent_id()),
      FromProto(source.reference_type_id()),
      source.has_attributes() ? FromProto(source.attributes())
                              : scada::NodeAttributes{},
      VectorFromProto<scada::NodeProperty>(source.property()),
      VectorFromProto<scada::ReferenceDescription>(source.reference()),
      VectorFromProto<scada::NodeState>(source.children()),
  };
}
