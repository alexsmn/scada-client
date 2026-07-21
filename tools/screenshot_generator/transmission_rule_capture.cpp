#include "transmission_rule_capture.h"

#include "screenshot_config.h"
#include "screenshot_wait.h"
#include "widget_capture.h"

#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "scada/node_id.h"
#include "transmission_rules/qt/transmission_rule_inspector.h"

#include <array>
#include <vector>

void SaveTransmissionRuleScreenshot(const ScreenshotSpec& spec,
                                    NodeService& node_service) {
  // TS.733 "TX_Ua" retransmits the analog source Ua to Modbus IOA 2001 under
  // the retransmission device TS.702 — the mockup's kind of rule (source → IOA)
  // rendered from real fixture data.
  const scada::NodeId rule_id = NodeIdFromScadaString("TS.733");

  // Wave 1: the rule + its children (the Address/SourceNode property instances) and
  // its direct type.
  scada::screenshot_generator::FetchNodesResident(node_service,
                                                  std::array{rule_id});

  NodeRef rule = node_service.GetNode(rule_id);

  // Wave 2: the full type chain (the Address/SourceNode declarations
  // live on the TransmissionItemType supertype, so operator[](declaration)
  // needs the chain's declarations resident), plus the parent endpoint
  // (destination device name).
  std::vector<scada::NodeId> extra;
  for (NodeRef type = rule.type_definition(); type; type = type.supertype())
    extra.push_back(type.node_id());
  if (NodeRef parent = rule.parent())
    extra.push_back(parent.node_id());
  scada::screenshot_generator::FetchNodesResident(node_service, extra);

  rule = node_service.GetNode(rule_id);

  // Wave 3: the source data item (display name + signal tag). The source link
  // is the SourceNode NodeId property (transmission OPC UA alignment,
  // phase 4), readable only now that the chain's declarations are resident.
  if (scada::NodeId source_id =
          rule[scada::devices::id::TransmissionItemType_SourceNode]
              .value()
              .get_or(scada::NodeId{});
      !source_id.is_null()) {
    scada::screenshot_generator::FetchNodesResident(node_service,
                                                    std::array{source_id});
    rule = node_service.GetNode(rule_id);
  }

  TransmissionRuleInspector inspector;
  inspector.ShowRule(rule);

  SaveScreenshot(&inspector, spec);
}
