#include "device_metrics_capture.h"

#include "publish_guard.h"
#include "screenshot_config.h"
#include "screenshot_wait.h"
#include "widget_capture.h"

#include "main_window/main_window.h"
#include "main_window/opened_view/opened_view.h"
#include "model/node_id_util.h"
#include "modules/device_metrics/device_metrics_command.h"
#include "modules/sheet/sheet_component.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "profile/window_definition.h"
#include "scada/node_id.h"

#include <gtest/gtest.h>

#include <array>
#include <vector>

void SaveDeviceMetricsScreenshot(const ScreenshotSpec& spec,
                                 MainWindow& main_window,
                                 NodeService& node_service,
                                 TimedDataService& timed_data_service,
                                 AnyExecutor executor) {
  CapturePublishGuard publish_guard{spec.filename};

  const scada::NodeId device_id = NodeIdFromScadaString(spec.path);
  if (device_id.is_null()) {
    ADD_FAILURE() << spec.filename << ": no device path in the spec";
    return;
  }

  // Wave 1: the device itself, its children (the diagnostic counter instances
  // the cells read) and its own type.
  scada::screenshot_generator::FetchNodesResident(node_service,
                                                  std::array{device_id});

  NodeRef device = node_service.GetNode(device_id);
  if (!device || !device.type_definition()) {
    ADD_FAILURE() << spec.filename << ": " << spec.path
                  << " is not a device with a type definition";
    return;
  }

  // Wave 2: the whole type chain. CollectVariables reads the data-variable
  // *declarations*, and for an IEC 60870 device those live on the DeviceType
  // supertype (Online/Enabled/MessagesIn/... in devices.xml), not on the
  // device's own type — so without the chain resident the sheet comes out with
  // a header column and no rows.
  std::vector<scada::NodeId> supertypes;
  for (NodeRef type = device.type_definition(); type; type = type.supertype())
    supertypes.push_back(type.node_id());
  scada::screenshot_generator::FetchNodesResident(node_service, supertypes);

  WindowDefinition window_definition =
      scada::screenshot_generator::WaitForAwaitable(
          executor, MakeDeviceMetricsWindowDefinitionAsync(executor, device));

  // A metrics sheet whose type declarations never became resident builds with
  // a header column and no metric cells, and renders as a bare frame — the
  // failure mode that has shipped as a "successful" capture before. Count the
  // value cells in the definition rather than rows in the widget: the sheet is
  // a fixed grid, so its model's rowCount says nothing about its content.
  if (spec.min_rows > 0) {
    int metric_cells = 0;
    for (const WindowItem& item : window_definition.items) {
      if (item.name_is("SheetCell") && item.GetString("text").starts_with('='))
        ++metric_cells;
    }
    EXPECT_GE(metric_cells, spec.min_rows) << spec.filename;
  }

  scada::screenshot_generator::WaitForAwaitable(
      executor, main_window.OpenView(window_definition));

  // OpenView hands back an OpenedViewInterface, which has no widget; find the
  // OpenedView it created. The sheet is a plain CusTable, so match on the
  // title the metrics builder set (the device's display name) rather than on
  // the window type, which `sheet.png` also uses.
  OpenedView* view = nullptr;
  for (OpenedView* candidate : main_window.opened_views()) {
    if (candidate->window_info().name == kSheetWindowInfo.name &&
        candidate->window_def().title == window_definition.title) {
      view = candidate;
      break;
    }
  }
  if (!view) {
    ADD_FAILURE() << spec.filename << ": the metrics view did not open";
    return;
  }

  // Every metric cell is a `=NodeId` formula resolved through the data
  // services, so let those reads land before the grab.
  EXPECT_TRUE(scada::screenshot_generator::WaitForPendingData(
      node_service, timed_data_service))
      << spec.filename;

  if (!publish_guard.ShouldPublish())
    return;

  SaveScreenshot(view->view(), spec);
}
