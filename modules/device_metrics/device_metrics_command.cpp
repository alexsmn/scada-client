#include "modules/device_metrics/device_metrics_command.h"

#include "aui/color.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "common/formula_util.h"
#include "controller/action.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/selection_model.h"
#include "controller/window_info.h"
#include "core/selection_command_context.h"
#include "main_window/main_window_interface.h"
#include "model/devices_node_ids.h"
#include "modules/device_metrics/node_collector.h"
#include "modules/sheet/sheet_component.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "resources/common_resources.h"
#include "ui/common/client_utils.h"

#include <utility>

DeviceMetricsModule::DeviceMetricsModule(DeviceMetricsModuleContext&& context)
    : DeviceMetricsModuleContext{std::move(context)} {
  ui_command_registry_.AddAction(Action{.command_id_ = ID_OPEN_DEVICE_METRICS,
                                        .category_ = CATEGORY_SPECIFIC,
                                        .title_ = Translate("Metrics")});
  selection_commands_.AddCommand(BasicCommand<SelectionCommandContext>{
      .command_id = ID_OPEN_DEVICE_METRICS,
      .execute_handler =
          [executor = executor_](const SelectionCommandContext& context) {
            CoSpawn(
                executor,
                [executor, &main_window = context.main_window,
                 node = context.selection.node()]() mutable -> Awaitable<void> {
                  auto window_definition =
                      co_await MakeDeviceMetricsWindowDefinitionAsync(executor,
                                                                      node);
                  co_await main_window.OpenView(window_definition);
                  co_return;
                });
          },
      .available_handler =
          [](const SelectionCommandContext& context) {
            return IsInstanceOf(context.selection.node(),
                                scada::devices::id::DeviceType);
          }});
}

WindowDefinition MakeDeviceMetricsWindowDefinitionSync(
    std::u16string title,
    std::span<const NodeRef> devices) {
  WindowDefinition win(kSheetWindowInfo);
  win.title = std::move(title);

  // Header column.
  {
    WindowItem& cell = win.AddItem("Column");
    cell.SetInt("ix", 1);
    cell.SetInt("width", 200);
  }

  const scada::aui::Color kHeaderColor = scada::aui::Rgba{227, 227, 227};

  // Header.
  auto data_variable_decls = CollectVariables(devices) | to_vector;
  for (size_t i = 0; i < data_variable_decls.size(); ++i) {
    WindowItem& cell = win.AddItem("SheetCell");
    cell.SetInt("row", i + 2);
    cell.SetInt("col", 1);
    cell.SetString("text", ToString16(data_variable_decls[i].display_name()));
    cell.SetString("color", scada::aui::ColorToString(kHeaderColor));
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
      cell.SetString("color", scada::aui::ColorToString(kHeaderColor));
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

Awaitable<WindowDefinition> MakeDeviceMetricsWindowDefinitionAsync(
    AnyExecutor executor,
    const NodeRef& device) {
  if (!device.type_definition()) {
    throw std::runtime_error{"Device type"};
  }

  auto devices = co_await CollectNodesRecursiveAsync(
      std::move(executor), device, scada::devices::id::DeviceType);
  auto title = ToString16(device.display_name());
  co_return MakeDeviceMetricsWindowDefinitionSync(std::move(title), devices);
}
