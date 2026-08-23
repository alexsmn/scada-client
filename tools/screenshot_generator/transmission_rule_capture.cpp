#include "transmission_rule_capture.h"

#include "screenshot_config.h"
#include "screenshot_wait.h"
#include "widget_capture.h"

#include "model/node_id_util.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "scada/node_id.h"
#include "transmission_rules/qt/transmission_rule_inspector.h"
#include "transmission_rules/transmission_rule_fetch.h"

#include <chrono>
#include <functional>
#include <utility>

void SaveTransmissionRuleScreenshot(const ScreenshotSpec& spec,
                                    NodeService& node_service,
                                    AnyExecutor executor) {
  // TS.733 "TX_Ua" retransmits the analog source Ua to Modbus IOA 2001 under
  // the retransmission device TS.702 — the mockup's kind of rule (source → IOA)
  // rendered from real fixture data.
  const scada::NodeId rule_id = NodeIdFromScadaString("TS.733");

  TransmissionRuleInspector inspector;
  // Wired the way the shell wires it, and for the reason the shell needs it.
  // Until 2026-08-23 this capture ran three FetchNodesResident waves of its own
  // and then handed ShowRule an already-resident rule, which made the published
  // image a claim about a panel nobody could reach: the shell fetched none of
  // that, so a rule an operator selected rendered "— → 0" — no source, no
  // signal tag, IOA 0. The panel asks for its own data now, and driving it
  // through that same seam is what keeps this image a true one.
  inspector.SetLoadHandler([executor](const NodeRef& rule,
                                      std::function<void()> redraw) {
    CoSpawn(executor, [rule, redraw = std::move(redraw)]() -> Awaitable<void> {
      co_await FetchTransmissionRule(rule);
      redraw();
    });
  });
  inspector.ShowRule(node_service.GetNode(rule_id));

  // The load is asynchronous now, so let it land before the grab.
  scada::screenshot_generator::PumpEventLoopFor(std::chrono::seconds(1));

  SaveScreenshot(&inspector, spec);
}
