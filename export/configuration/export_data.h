#pragma once

#include "base/struct_writer.h"
#include "common/node_state.h"

struct ExportData {
  struct Property {
    scada::NodeId node_id;
    scada::LocalizedText display_name;
    bool reference = false;

    bool operator==(const Property&) const = default;
  };

  // TODO: Remove |node_id| and |reference|.
  struct PropertyValue {
    scada::NodeId node_id;
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
      .AddField("node_id", prop.node_id)
      .AddField("display_name", prop.display_name)
      .AddField("reference", prop.reference);
  return os;
}

inline std::ostream& operator<<(std::ostream& os,
                                const ExportData::PropertyValue& value) {
  StructWriter{os}
      .AddField("node_id", value.node_id)
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
