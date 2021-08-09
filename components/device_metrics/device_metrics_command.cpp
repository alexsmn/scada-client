#include "components/device_metrics/device_metrics_command.h"

#include "client_utils.h"
#include "common/formula_util.h"
#include "common_resources.h"
#include "components/device_metrics/node_collector.h"
#include "components/sheet/sheet_component.h"
#include "controls/color.h"
#include "model/devices_node_ids.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "window_info.h"

WindowDefinition MakeDeviceMetricsWindowDefinitionSync(
    std::wstring title,
    base::span<const NodeRef> devices) {
  WindowDefinition win(kSheetWindowInfo);
  win.title = std::move(title);

  // Header column.
  {
    WindowItem& cell = win.AddItem("Column");
    cell.SetInt("ix", 1);
    cell.SetInt("width", 200);
  }

  const aui::Color kHeaderColor = aui::Rgba{227, 227, 227};

  // Header.
  auto data_variable_decls = CollectVariables(devices) | to_vector;
  for (size_t i = 0; i < data_variable_decls.size(); ++i) {
    WindowItem& cell = win.AddItem("SheetCell");
    cell.SetInt("row", i + 2);
    cell.SetInt("col", 1);
    cell.SetString("text", ToString16(data_variable_decls[i].display_name()));
    cell.SetString("color", aui::ColorToString(kHeaderColor));
    cell.SetString("align", "right");
  }

  // Items.
  for (size_t i = 0; i < devices.size(); ++i) {
    const auto& device = devices[i];

    // Item header.
    {
      WindowItem& cell = win.AddItem("SheetCell");
      cell.SetInt("row", 1);
      cell.SetInt("col", i + 2);
      cell.SetString("text", ToString16(device.display_name()));
      cell.SetString("color", aui::ColorToString(kHeaderColor));
    }

    // Metric cells.
    for (size_t j = 0; j < data_variable_decls.size(); ++j) {
      if (auto data_variable = device[data_variable_decls[j].browse_name()]) {
        auto& cell = win.AddItem("SheetCell");
        cell.SetInt("row", j + 2);
        cell.SetInt("col", i + 2);
        auto formula = MakeNodeIdFormula(data_variable.node_id());
        cell.SetString("text", '=' + formula);
      }
    }
  }

  return win;
}

promise<WindowDefinition> MakeDeviceMetricsWindowDefinition(
    const NodeRef& device) {
  if (!device.type_definition()) {
    return make_rejected_promise<WindowDefinition>(
        std::runtime_error{"Device type"});
  }

  return CollectNodesRecursive(device, devices::id::DeviceType)
      .then([device](const std::vector<NodeRef>& devices) {
        auto title = ToString16(device.display_name());
        return MakeDeviceMetricsWindowDefinitionSync(std::move(title), devices);
      });
}
