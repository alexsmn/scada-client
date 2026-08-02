#include "device_diagnostics_capture.h"

#include "screenshot_config.h"
#include "screenshot_wait.h"
#include "widget_capture.h"

#include "aui/translation.h"
#include "device_diagnostics/qt/device_diagnostics_panel.h"
#include "model/node_id_util.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "scada/node_id.h"

#include <array>
#include <chrono>
#include <string_view>
#include <utility>

void SaveDeviceDiagnosticsScreenshot(const ScreenshotSpec& spec,
                                     NodeService& node_service,
                                     TimedDataService& timed_data_service,
                                     const boost::json::value& json) {
  // The fixture device carrying seeded diagnostic child variables (link down +
  // traffic/polling counters) — КП-01, an IEC 60870-5-104 RTU, matching the
  // mockup's "Link down" hero.
  const scada::NodeId device_id = NodeIdFromScadaString("TS.103");

  // Make the device (its children + type definition + values) resident so
  // ShowDevice can resolve the diagnostic variables and read their values.
  const std::array<scada::NodeId, 1> ids{device_id};
  scada::screenshot_generator::FetchNodesResident(node_service, ids);

  NodeRef device = node_service.GetNode(device_id);

  // Demo actions so the capture shows the Actions row (the app wires these to
  // real device commands; here they are inert but enabled).
  //
  // "Reconnect now" is deliberately NOT in this list. Since ADR 0007 it is the
  // protocol registry's link action — an OPC UA Method on the device's parent
  // link, offered only when that link exists — so hard-coding its label here
  // would put a button in the manual that the app does not show for this
  // device. The call path below is wired inert so the capture takes the same
  // route the app does; this fixture parents КП-01 straight onto the Devices
  // folder, so no link resolves and the panel omits both the link section and
  // its action. Modelling a link in the fixture is what would bring them back.
  DeviceDiagnosticsPanelContext context;
  for (std::string_view label : {"Metrics trend", "Open log"})
    context.actions.push_back(DiagnosticAction{.label = Translate(label)});
  context.call_link_method = [](const NodeRef&, const scada::NodeId&) {};
  DeviceDiagnosticsPanel panel{std::move(context)};
  panel.ShowDevice(device, timed_data_service);

  // Let the current-value fetch chains settle so the counters and hero populate
  // before the grab (see SaveSeriesInspectorScreenshot).
  scada::screenshot_generator::PumpEventLoopFor(std::chrono::seconds(1));

  SaveScreenshot(&panel, spec);
}
