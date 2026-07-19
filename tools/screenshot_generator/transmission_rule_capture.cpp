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

  // Wave 1: the rule + its children (the SourceAddress property instance) and
  // its direct type.
  scada::screenshot_generator::FetchNodesResident(node_service,
                                                  std::array{rule_id});

  NodeRef rule = node_service.GetNode(rule_id);

  // Wave 2: the full type chain (the SourceAddress declaration lives on the
  // TransmissionItemType supertype, so operator[](SourceAddress) needs the
  // chain's declarations resident), plus the source data item (display name +
  // signal tag) and the parent endpoint (destination device name).
  std::vector<scada::NodeId> extra;
  for (NodeRef type = rule.type_definition(); type; type = type.supertype())
    extra.push_back(type.node_id());
  if (NodeRef source =
          rule.target(scada::devices::id::HasTransmissionSource))
    extra.push_back(source.node_id());
  if (NodeRef parent = rule.parent())
    extra.push_back(parent.node_id());
  scada::screenshot_generator::FetchNodesResident(node_service, extra);

  rule = node_service.GetNode(rule_id);

  TransmissionRuleInspector inspector;
  inspector.ShowRule(rule);

  SaveScreenshot(&inspector, spec);
}
