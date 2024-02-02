#pragma once

#include "common/node_state.h"

struct ExportData {
  struct Property {
    scada::NodeId node_id;
    scada::LocalizedText display_name;
    bool reference = false;
  };

  // TODO: Remove |node_id| and |reference|.
  struct PropertyValue {
    scada::NodeId node_id;
    scada::Variant value;
    scada::NodeId target_id;
    scada::LocalizedText target_display_name;
    bool reference = false;
  };

  struct Node {
    scada::NodeId node_id;
    scada::NodeId parent_id;
    scada::LocalizedText type_display_name;
    scada::NodeId type_id;
    scada::LocalizedText display_name;
    std::vector<PropertyValue> property_values;
  };

  std::vector<Property> props;
  std::vector<Node> nodes;
};
