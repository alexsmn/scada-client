#pragma once

#include "base/struct_writer.h"
#include "common/node_state.h"

struct ExportData {
  struct Property {
    // For references represents the reference type ID.
    scada::NodeId prop_decl_id;
    scada::LocalizedText display_name;
    bool reference = false;

    bool operator==(const Property&) const = default;
  };

  // TODO: Remove |node_id| and |reference|.
  struct PropertyValue {
    scada::NodeId prop_decl_id;
    scada::Variant value;
    scada::NodeId target_id;
    scada::LocalizedText target_display_name;
    bool reference = false;

    bool operator==(const PropertyValue&) const = default;
  };

  struct Node {
    scada::NodeId node_id;
    scada::NodeId parent_id;
    scada::LocalizedText type_display_name;
    scada::NodeId type_id;
    scada::LocalizedText display_name;
    std::vector<PropertyValue> property_values;

    const PropertyValue* FindPropValue(
        const scada::NodeId& prop_decl_id) const {
      auto i = std::ranges::find(property_values, prop_decl_id,
                                 &PropertyValue::prop_decl_id);
      return i != property_values.end() ? std::to_address(i) : nullptr;
    }

    // Return an empty variant when not found.
    scada::Variant GetPropValue(const scada::NodeId& prop_decl_id) const {
      if (auto* value = FindPropValue(prop_decl_id)) {
        return value->value;
      }
      return {};
    }

    // Returns the null node ID when not found.
    scada::NodeId GetTargetId(const scada::NodeId& ref_type_id) const {
      if (auto* value = FindPropValue(ref_type_id)) {
        return value->target_id;
      }
      return {};
    }

    bool operator==(const Node&) const = default;
  };

  std::vector<Property> props;
  std::vector<Node> nodes;

  bool operator==(const ExportData&) const = default;
};

inline std::ostream& operator<<(std::ostream& os,
                                const ExportData& export_data) {
  StructWriter{os}
      .AddField("props", export_data.props)
      .AddField("nodes", export_data.nodes);
  return os;
}

inline std::ostream& operator<<(std::ostream& os,
                                const ExportData::Property& prop) {
  StructWriter{os}
      .AddField("prop_decl_id", prop.prop_decl_id)
      .AddField("display_name", prop.display_name)
      .AddField("reference", prop.reference);
  return os;
}

inline std::ostream& operator<<(std::ostream& os,
                                const ExportData::PropertyValue& value) {
  StructWriter{os}
      .AddField("prop_decl_id", value.prop_decl_id)
      .AddField("value", value.value)
      .AddField("target_id", value.target_id)
      .AddField("target_display_name", value.target_display_name)
      .AddField("reference", value.reference);
  return os;
}

inline std::ostream& operator<<(std::ostream& os,
                                const ExportData::Node& node) {
  StructWriter{os}
      .AddField("node_id", node.node_id)
      .AddField("parent_id", node.parent_id)
      .AddField("type_display_name", node.type_display_name)
      .AddField("type_id", node.type_id)
      .AddField("display_name", node.display_name)
      .AddField("property_values", node.property_values);
  return os;
}
