#include "client_utils.h"
#include "common/formula_util.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "common_resources.h"
#include "controls/color.h"
#include "model/devices_node_ids.h"
#include "model/scada_node_ids.h"
#include "window_info.h"

namespace {

struct DeviceMetricsItem {
  NodeRef node;
  int level;
};

void DeviceMetricsItems(const NodeRef& node,
                        std::vector<DeviceMetricsItem>& rows) {
  if (!IsInstanceOf(node, devices::id::DeviceType))
    return;

  rows.push_back({node, 0});

  for (const auto& n : node.targets(scada::id::Organizes))
    DeviceMetricsItems(n, rows);
}

}  // namespace

std::optional<WindowDefinition> MakeDeviceMetricsWindowDefinition(
    const NodeRef& device) {
  if (!device.type_definition())
    return std::nullopt;

  std::vector<DeviceMetricsItem> items;
  DeviceMetricsItems(device, items);

  WindowDefinition win(GetWindowInfo(ID_SHEET_VIEW));
  win.title = ToString16(device.display_name());

  // Header column.
  {
    WindowItem& cell = win.AddItem("Column");
    cell.SetInt("ix", 1);
    cell.SetInt("width", 200);
  }

  const aui::Color kHeaderColor = aui::Rgba{227, 227, 227};

  // Header.
  auto components = GetDataVariables(device);
  for (size_t i = 0; i < components.size(); ++i) {
    WindowItem& cell = win.AddItem("SheetCell");
    cell.SetInt("row", i + 2);
    cell.SetInt("col", 1);
    cell.SetString("text", ToString16(components[i].display_name()));
    cell.SetString("color", aui::ColorToString(kHeaderColor));
    cell.SetString("align", "right");
  }

  // Items.
  for (size_t i = 0; i < items.size(); ++i) {
    const DeviceMetricsItem& item = items[i];

    // Item header.
    {
      WindowItem& cell = win.AddItem("SheetCell");
      cell.SetInt("row", 1);
      cell.SetInt("col", i + 2);
      cell.SetString("text", ToString16(item.node.display_name()));
      cell.SetString("color", aui::ColorToString(kHeaderColor));
    }

    // Metric cells.
    for (size_t j = 0; j < components.size(); ++j) {
      if (auto variable = device[components[j].browse_name()]) {
        WindowItem& cell = win.AddItem("SheetCell");
        cell.SetInt("row", j + 2);
        cell.SetInt("col", i + 2);
        cell.SetString("text", '=' + MakeNodeIdFormula(variable.node_id()));
      }
    }
  }

  return win;
}
