#include "graph_capture.h"

#include "screenshot_config.h"
#include "screenshot_output.h"
#include "screenshot_wait.h"
#include "widget_capture.h"

#include "base/time_utils.h"
#include "graph/metrix_graph.h"
#include "graph/series_inspector.h"
#include "profile/window_definition.h"
#include "scada/node_id.h"
#include "timed_data/timed_data_spec.h"

#include <QApplication>
#include <QColor>
#include <QPixmap>
#include <QString>

#include <chrono>
#include <map>
#include <string>
#include <vector>

WindowDefinition MakeGraphDefinition(const boost::json::value& json) {
  WindowDefinition def{"Graph"};
  const auto& graph = json.at("graph").as_object();

  for (const auto& jp : graph.at("panes").as_array()) {
    auto& item = def.AddItem("GraphPane");
    item.SetInt("ix", static_cast<int>(jp.at("index").as_int64()));
    item.SetInt("size", static_cast<int>(jp.at("size").as_int64()));
    if (auto* act = jp.as_object().if_contains("active"); act && act->as_bool())
      item.SetInt("act", 1);
  }

  for (const auto& ji : graph.at("items").as_array()) {
    def.AddItem("Item")
        .SetString("path", std::string(ji.at("path").as_string()))
        .SetString("clr", std::string(ji.at("color").as_string()))
        .SetInt("pane", static_cast<int>(ji.at("pane").as_int64()))
        .SetInt("dots", ji.at("dots").as_bool() ? 1 : 0)
        .SetInt("stepped", ji.at("stepped").as_bool() ? 1 : 0);
  }

  const auto& ts = graph.at("time_scale").as_object();
  def.AddItem("TimeScale")
      .SetString("time", std::string(ts.at("time").as_string()))
      .SetString("span", std::string(ts.at("span").as_string()))
      .SetBool("scrollBar", ts.at("scroll_bar").as_bool());

  return def;
}

namespace {

// Resolves each graphed item's node id from its timed-data formula and makes
// those nodes fully resident (own attributes + property children) via the
// node service, before any line is built. MetrixDataSource reads the EU range,
// current value and limit bands off the node (`node[declaration_id].value()`)
// in UpdateRange/UpdateLimits, which run from OnItemChanged the moment
// NewLine() connects the line. The standalone graph widget is built outside
// the main-window/tree flow that would otherwise pull those property children
// resident, and TimedData only fetches the node itself (NodeOnly) — so without
// this the axes fall back to the data auto-range and the current/limit readouts
// come up blank. Must run before BuildGraphFromJson().
void MakeGraphItemNodesResident(NodeService& node_service,
                                TimedDataService& timed_data_service,
                                const boost::json::value& json) {
  std::vector<scada::NodeId> node_ids;
  for (const auto& ji : json.at("graph").as_object().at("items").as_array()) {
    TimedDataSpec probe;
    probe.Connect(timed_data_service, std::string(ji.at("path").as_string()));
    node_ids.push_back(probe.node_id());
  }
  scada::screenshot_generator::FetchNodesResident(node_service, node_ids);
}

// Builds the fixture graph — panes, coloured lines and the time range — into
// `graph` from the JSON `graph` section, and pulls the timed data. Shared by
// the full-graph and series-inspector captures.
void BuildGraphFromJson(MetrixGraph& graph, const boost::json::value& json) {
  const auto& jgraph = json.at("graph").as_object();

  // Panes.
  std::map<int, MetrixGraph::MetrixPane*> pane_map;
  for (const auto& jp : jgraph.at("panes").as_array()) {
    auto& pane = graph.NewPane();
    int ix = static_cast<int>(jp.at("index").as_int64());
    pane.size_percent_ = static_cast<int>(jp.at("size").as_int64());
    pane_map[ix] = &pane;
    if (auto* act = jp.as_object().if_contains("active"); act && act->as_bool())
      graph.SelectPane(&pane);
  }

  // Lines.
  for (const auto& ji : jgraph.at("items").as_array()) {
    auto path = std::string(ji.at("path").as_string());
    int pane_ix = static_cast<int>(ji.at("pane").as_int64());
    auto* pane = pane_map[pane_ix];
    auto& line = graph.NewLine(path, *pane);
    line.SetColor(QColor(
        QString::fromStdString(std::string(ji.at("color").as_string()))));
    line.set_dots_shown(ji.at("dots").as_bool());
    line.set_stepped(ji.at("stepped").as_bool());
  }

  // Time range (span parsed from "HH:MM:SS"). Anchor "now" to the fixture's
  // frozen clock when present so regenerated axis labels stay stable across
  // runs; LocalHistoryService reads the same key.
  auto now = base::Time::Now();
  if (const auto* jnow = json.as_object().if_contains("now")) {
    base::Time fixed_now;
    if (base::Time::FromString(std::string(jnow->as_string()).c_str(),
                               &fixed_now))
      now = fixed_now;
  }
  auto span_str = std::string(jgraph.at("time_scale").at("span").as_string());
  base::TimeDelta span;
  Deserialize(span_str, span);
  double from = (now - span).ToDoubleT();
  double to = now.ToDoubleT();
  graph.horizontal_axis().SetTimeFit(false);
  graph.horizontal_axis().SetRange(
      views::GraphRange{from, to, views::GraphRange::TIME});

  graph.UpdateData();
}

}  // namespace

void SaveGraphScreenshot(const ScreenshotSpec& spec,
                         NodeService& node_service,
                         TimedDataService& timed_data_service,
                         const boost::json::value& json) {
  MakeGraphItemNodesResident(node_service, timed_data_service, json);

  MetrixGraph graph{MetrixGraphContext{timed_data_service}};
  BuildGraphFromJson(graph, json);

  for (auto* pane : graph.panes())
    static_cast<MetrixGraph::MetrixPane*>(pane)->ShowLegend(true);

  // Let the lines' async history reads and current-value subscribe/read chains
  // (routed through the executor + Qt event loop) settle so the plotted series,
  // the legend current/min/max/average cells and the cursor readout are
  // populated before the grab. A standalone graph doesn't get the incidental
  // pumping a full capture run accumulates, so it must pump for itself.
  scada::screenshot_generator::PumpEventLoopFor(std::chrono::seconds(1));

  // Render — matches graph_qt's RenderWidget pattern exactly.
  graph.setFixedSize(spec.width, spec.height);
  graph.show();
  scada::screenshot_generator::PumpEventLoopFor(std::chrono::milliseconds(200));

  QPixmap pixmap = graph.grab();
  auto output_path = GetOutputDir() / spec.filename;
  pixmap.save(QString::fromStdString(output_path.string()));
}

void SaveSeriesInspectorScreenshot(const ScreenshotSpec& spec,
                                   NodeService& node_service,
                                   TimedDataService& timed_data_service,
                                   const boost::json::value& json) {
  MakeGraphItemNodesResident(node_service, timed_data_service, json);

  MetrixGraph graph{MetrixGraphContext{timed_data_service}};
  BuildGraphFromJson(graph, json);

  // Let the async history/current-value chains settle so the inspector's
  // current, min/max/average and limit rows are populated before the grab
  // (see SaveGraphScreenshot).
  scada::screenshot_generator::PumpEventLoopFor(std::chrono::seconds(1));

  // Point at the first pane's series (the first graphed item)
  // deterministically: NewPane() auto-selects the last-created pane, so
  // graph.primary_line() would otherwise follow the bottom pane rather than the
  // top one.
  SeriesInspector inspector;
  if (!graph.panes().empty()) {
    inspector.SetLine(
        static_cast<MetrixGraph::MetrixPane*>(graph.panes().front())
            ->primary_line());
  }
  SaveScreenshot(&inspector, spec);
}
