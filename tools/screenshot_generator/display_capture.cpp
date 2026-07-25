#include "display_capture.h"

#include "screenshot_config.h"
#include "screenshot_output.h"
#include "widget_capture.h"
#include "screenshot_wait.h"

#include "base/client_paths.h"
#include "base/path_service.h"
#include "common/vds_runtime_api.h"
#include "display_frame/qt/display_frame.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"
#include "vds_runtime/qt/vds_runtime_widget.h"

#include <QApplication>
#include <QPixmap>
#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace {

// The display document to render, resolved from the manifest's `display.path`
// (relative to the fixture directory, next to screenshot_data.json).
std::filesystem::path FixturePath(const boost::json::value& json) {
  std::string rel = "fixtures/substation.vds";
  if (const auto* display = json.as_object().if_contains("display")) {
    if (const auto* path = display->as_object().if_contains("path"))
      rel = std::string(path->as_string());
  }
  std::filesystem::path result{rel};
  if (result.is_relative())
    result = GetDataFilePath().parent_path() / result;
  return result;
}

// The fixture signals pushed into the Measurements strip, from the manifest's
// `display.measurements` (defaulting to a representative pair).
std::vector<std::string> MeasurementPaths(const boost::json::value& json) {
  std::vector<std::string> paths;
  if (const auto* display = json.as_object().if_contains("display")) {
    if (const auto* values = display->as_object().if_contains("measurements")) {
      for (const auto& value : values->as_array())
        paths.emplace_back(value.as_string());
    }
  }
  return paths;
}

}  // namespace

void SaveDisplayScreenshot(const ScreenshotSpec& spec,
                           const boost::json::value& json,
                           TimedDataService& timed_data_service,
                           NodeEventProvider& node_event_provider,
                           NodeService& node_service) {
  // The VDS runtime dylib is loaded from the client install dir. The generator
  // never sets base::DIR_EXE, so point client::DIR_INSTALL at the binary dir
  // (where the dylib is co-located) so the renderer resolves it and paints the
  // real document instead of its "runtime unavailable" placeholder.
  scada::base::PathService::Override(
      client::DIR_INSTALL,
      std::filesystem::path{QApplication::applicationDirPath().toStdString()});

  // The DisplayFrame reparents (owns) the renderer, so the frame is the single
  // owning widget we render and delete.
  auto* diagram = new VdsRuntimeWidget;
  diagram->Open(FixturePath(json), TC_VDS_RUNTIME_DOCUMENT_KIND_AUTO);

  // The bay strips ride the app's live services, so the capture shows the
  // whole reshelled surface rather than just the chrome: the Recent-events
  // list fills from the fixture's pending alarms, and the signals below stand
  // in for the elements an operator would click on the diagram.
  QWidget* frame = WrapDisplayInFrame(
      diagram, diagram->title(),
      DisplayFrameContext{.timed_data_service = &timed_data_service,
                          .node_event_provider = &node_event_provider,
                          .node_service = &node_service});
  std::unique_ptr<QWidget> owner{frame};

  if (auto* display_frame = qobject_cast<DisplayFrame*>(frame)) {
    std::vector<scada::NodeId> node_ids;
    for (const std::string& path : MeasurementPaths(json))
      node_ids.push_back(NodeIdFromScadaString(path));
    // Same residency rule as the graph capture: a standalone widget is built
    // outside the main-window/tree flow that would pull a node's attributes
    // and property children resident, and TimedData fetches only the node
    // itself — so without this the strip lists the signals with blank values.
    scada::screenshot_generator::FetchNodesResident(node_service, node_ids);
    for (const scada::NodeId& node_id : node_ids)
      display_frame->ShowMeasurement(node_id);
  }

  frame->setFixedSize(spec.width, spec.height);
  frame->show();
  // The Measurements rows fill from monitored-item deliveries, which arrive on
  // the loop after the specs connect — a single processEvents would grab the
  // strip before any value lands.
  scada::screenshot_generator::PumpEventLoopFor(std::chrono::milliseconds(400));

  // Guard the Measurements strip's live-value delivery: the current value
  // arrives as a PROPERTY_CURRENT change, so the frame must refresh on the
  // spec's property_change_handler, not only its update_handler (a
  // current-only spec produces no buffer updates at all). If that regresses
  // the Value column goes blank again, which the render alone would not catch.
  if (auto* table = frame->findChild<QTableWidget*>(
          QStringLiteral("displayMeasurements"));
      table && table->rowCount() > 0) {
    QTableWidgetItem* value = table->item(0, 1);
    EXPECT_TRUE(value && !value->text().isEmpty())
        << "Measurements strip value cell is blank - the current value did "
           "not reach the row";
  }

  QPixmap pixmap = GrabWhenSettled(frame);
  auto output_path = GetOutputDir() / spec.filename;
  pixmap.save(QString::fromStdString(output_path.string()));
}
