#include "display_capture.h"

#include "publish_guard.h"
#include "screenshot_config.h"
#include "screenshot_output.h"
#include "screenshot_wait.h"
#include "widget_capture.h"

#include "display_frame/qt/display_frame.h"
#include "display_view/display_runtime.h"
#include "display_view/qt/display_widget.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"

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
                           AnyExecutor executor,
                           const boost::json::value& json,
                           TimedDataService& timed_data_service,
                           NodeEventProvider& node_event_provider,
                           NodeService& node_service) {
  CapturePublishGuard publish_guard{spec.filename};

  // **The precondition, stated rather than assumed.** Since ADR 0013 the
  // schematic is drawn by a display runtime loaded at run time, and a client
  // configured without one is a SUPPORTED state: `DisplayWidget` lays out
  // perfectly and draws "there is no display runtime" over an empty page. That
  // is a valid PNG of a missing dependency, and nothing downstream can tell it
  // from a real render — right dimensions, right layout, no empty-surface
  // tell, which is exactly the case `publish_guard.h` says no dimension or
  // layout check can catch.
  //
  // So it FAILED OPEN: `client/`'s configure leaves
  // `SCADA_DISPLAY_RUNTIME_LIBRARY` empty by default, the generator exited 0,
  // and a routine full pass overwrote the tracked capture with the placeholder
  // — 402107 differing pixels — unless a human happened to look at the image
  // (backlog 814).
  //
  // SKIP rather than fail, and the distinction is ADR 0013's: a client with no
  // runtime beside it must build and pass its own suite, because that is the
  // state a stranger who cloned the public repository is in, and `display` is
  // never published. A hard failure here would make an optional dependency a
  // test requirement and turn the client's own CI red for a supported
  // configuration. What matters for 814 is that nothing is WRITTEN — the
  // tracked capture keeps its bytes, the skip is visible in the run, and
  // `check_screenshots.py` reports the row as owed rather than produced.
  if (!DisplayRuntime::Get()) {
    GTEST_SKIP()
        << spec.filename << " renders through the display runtime, and none "
           "was loaded: "
        << DisplayRuntime::unavailable_reason()
        << "\nThe tracked capture was left alone rather than overwritten with "
           "the placeholder. To produce it, build the `display` product and "
           "configure the client with"
           "\n  -DSCADA_DISPLAY_RUNTIME_LIBRARY=<display>/build/ninja/bin/"
           "<Config>/libdisplay_runtime.<so|dylib|dll>"
           "\nor set SCADA_DISPLAY_RUNTIME in the environment.";
  }

  // The DisplayFrame reparents (owns) the renderer, so the frame is the single
  // owning widget we render and delete.
  auto* diagram = new DisplayWidget;
  diagram->Open(FixturePath(json), DisplayDocumentKind::kAuto);

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
    scada::screenshot_generator::FetchNodesResident(executor, node_service,
                                                    node_ids);
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
  auto output_path = OutputPathFor(spec.filename);
  if (!publish_guard.ShouldPublish())
    return;

  pixmap.save(QString::fromStdString(output_path.string()));
}
