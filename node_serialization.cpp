#include "node_serialization.h"

#include "common/node_state.h"
#include "core/standard_node_ids.h"
#include "node_serialization.h"
#include "node_service/node_util.h"
#include "remote/protocol_utils.h"

void NodeToData(const NodeRef& source,
                scada::NodeState& target,
                bool recursive,
                bool ignore_browse_name) {
  assert(source.node_class().has_value());

  target.node_id = source.node_id();
  target.node_class = source.node_class().value();

  const auto& parent_ref =
      source.inverse_reference(scada::id::HierarchicalReferences);
  target.parent_id = parent_ref.target.node_id();
  target.reference_type_id = parent_ref.reference_type.node_id();

  if (auto type_definition = source.type_definition())
    target.type_definition_id = type_definition.node_id();

  if (!ignore_browse_name)
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

  // Skip type definitions, HasComponent.
  for (auto& ref : source.references(scada::id::NonHierarchicalReferences)) {
    assert(ref.forward);
    if (!IsSubtypeOf(ref.reference_type, scada::id::HasTypeDefinition)) {
      target.references.push_back(
          {ref.reference_type.node_id(), ref.forward, ref.target.node_id()});
    }
  }

  if (recursive) {
    for (const auto& child : source.targets(scada::id::Organizes)) {
      NodeToData(child, target.children.emplace_back(), true,
                 ignore_browse_name);
    }
  }
}

template <>
void Convert(const protocol::NodeReference& source,
             scada::ReferenceDescription& target) {
  Convert(source.reference_type_id(), target.reference_type_id);
  target.forward = source.forward();
  Convert(source.node_id(), target.node_id);
}

template <>
void Convert(const scada::ReferenceDescription& source,
             protocol::NodeReference& target) {
  Convert(source.reference_type_id, *target.mutable_reference_type_id());
  target.set_forward(source.forward);
  Convert(source.node_id, *target.mutable_node_id());
}

template <>
void Convert(const protocol::NodeProperty& source,
             scada::NodeProperty& target) {
  Convert(source.declaration_id(), target.first);
  Convert(source.value(), target.second);
}

template <>
void Convert(const scada::NodeProperty& source,
             protocol::NodeProperty& target) {
  Convert(source.first, *target.mutable_declaration_id());
  Convert(source.second, *target.mutable_value());
}

void Convert(const scada::NodeState& source, protocol::Node& target) {
  if (!source.parent_id.is_null())
    Convert(source.parent_id, *target.mutable_parent_id());
  else
    target.clear_parent_id();

  if (!source.reference_type_id.is_null())
    Convert(source.reference_type_id, *target.mutable_reference_type_id());
  else
    target.clear_reference_type_id();

  Convert(source.node_id, *target.mutable_node_id());
  target.set_node_class(ConvertTo<protocol::NodeClass>(source.node_class));

  if (!source.type_definition_id.is_null()) {
    assert(!scada::IsTypeDefinition(source.node_class));
    Convert(source.type_definition_id, *target.mutable_type_definition_id());
  } else
    target.clear_type_definition_id();

  Convert(source.attributes, *target.mutable_attributes());
  Convert(source.properties, *target.mutable_property());
  Convert(source.references, *target.mutable_reference());

  if (!source.children.empty())
    Convert(source.children, *target.mutable_children());
}

void Convert(const protocol::Node& source, scada::NodeState& target) {
  Convert(source.node_id(), target.node_id);
  Convert(source.node_class(), target.node_class);
  Convert(source.type_definition_id(), target.type_definition_id);
  Convert(source.parent_id(), target.parent_id);
  Convert(source.reference_type_id(), target.reference_type_id);
  if (source.has_attributes())
    Convert(source.attributes(), target.attributes);
  else
    target.attributes = {};
  Convert(source.property(), target.properties);
  Convert(source.reference(), target.references);
  Convert(source.children(), target.children);
}
