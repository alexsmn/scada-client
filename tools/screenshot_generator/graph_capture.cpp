#include "graph_capture.h"

#include "base/time/time_wire_codec.h"

#include "fixture_builder.h"
#include "publish_guard.h"
#include "screenshot_config.h"
#include "screenshot_output.h"
#include "screenshot_wait.h"
#include "widget_capture.h"

#include "base/time_utils.h"
#include "controller/selection_model.h"
#include "graph/metrix_graph.h"
#include "inspector/qt/inspector_panel.h"
#include "profile/window_definition.h"
#include "scada/node_id.h"
#include "timed_data/timed_data_spec.h"

#include <gtest/gtest.h>

#include <QApplication>
#include <QColor>
#include <QElapsedTimer>
#include <QPixmap>
#include <QString>

#include <algorithm>
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

// Returns the graph configuration a capture plots: the fixture's top-level
// `graphs` entry the spec names, or the fixture-wide `graph` object when it
// names none.
//
// A capture that plots the shared object renders the shared picture. That is
// how `graph-cursor.png` and `limits-chart.png` came to be the same file byte
// for byte: the two specs differed only in `filename`, so each was a superset
// of both subjects — a cursor the limits capture never asked for, limit bands
// the cursor capture never asked for — and neither illustrated its own.
const boost::json::object& ResolveGraphConfig(const ScreenshotSpec& spec,
                                              const boost::json::value& json) {
  const auto& root = json.as_object();
  if (spec.graph_config.empty())
    return root.at("graph").as_object();

  const auto* graphs = root.if_contains("graphs");
  const auto* config =
      graphs ? graphs->as_object().if_contains(spec.graph_config) : nullptr;
  if (!config) {
    ADD_FAILURE() << "screenshot " << spec.filename << " names graph config \""
                  << spec.graph_config
                  << "\", which the fixture's `graphs` "
                     "object does not define";
    return root.at("graph").as_object();
  }
  return config->as_object();
}

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
//
// The returned probes must be kept alive until after the grab. Residency is
// bounded and reference-counted, so probes destroyed at the end of this
// function let the nodes be evicted again before the lines read their
// properties — which showed up as an occasional capture whose series were
// correct but whose panes had auto-ranged (no EU band, no limit markers).
[[nodiscard]] std::vector<TimedDataSpec> MakeGraphItemNodesResident(
    NodeService& node_service,
    TimedDataService& timed_data_service,
    const boost::json::object& graph) {
  std::vector<TimedDataSpec> probes;
  for (const auto& ji : graph.at("items").as_array()) {
    TimedDataSpec& probe = probes.emplace_back();
    probe.Connect(timed_data_service, std::string(ji.at("path").as_string()));
  }

  // Connect() resolves the formula to a node asynchronously, so reading
  // node_id() straight after it can yield a null id — which FetchNodesResident
  // silently skips, leaving nothing resident. That is invisible in the output
  // except as a pane that auto-ranged instead of using its engineering-unit
  // band, and it only bit the first graph capture of a run (a later one found
  // the node already resolved), which is exactly the kind of order dependence
  // these captures must not have.
  auto ids_resolved = [&probes] {
    return std::ranges::none_of(probes, [](const TimedDataSpec& probe) {
      return probe.node_id().is_null();
    });
  };
  QElapsedTimer elapsed;
  elapsed.start();
  while (!ids_resolved() && !elapsed.hasExpired(10'000)) {
    scada::screenshot_generator::PumpEventLoopFor(
        std::chrono::milliseconds(50));
  }

  std::vector<scada::NodeId> node_ids;
  for (const TimedDataSpec& probe : probes)
    node_ids.push_back(probe.node_id());
  if (!ids_resolved()) {
    ADD_FAILURE() << "graph item formulas never resolved to node ids; the "
                     "panes would auto-range instead of using their "
                     "engineering-unit bands";
  }
  scada::screenshot_generator::FetchNodesResident(node_service, node_ids);
  return probes;
}

// Builds the fixture graph — panes, coloured lines and the time range — into
// `graph` from the resolved graph configuration `jgraph`, and pulls the timed
// data. `json` is the whole fixture, read for the frozen clock alone. Shared
// by the full-graph and series-inspector captures.
void BuildGraphFromJson(MetrixGraph& graph,
                        const boost::json::object& jgraph,
                        const boost::json::value& json) {
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
  // runs; LocalHistoryService reads the same key. (The generator fixture also
  // freezes scada::Time at this instant, so the fallback matches.)
  auto now = FixtureNow(json);
  if (scada::IsNull(now))
    now = scada::Now();
  auto span_str = std::string(jgraph.at("time_scale").at("span").as_string());
  scada::Duration span;
  Deserialize(span_str, span);
  double from = scada::base::EncodeDoubleT((now - span));
  double to = scada::base::EncodeDoubleT(now);
  graph.horizontal_axis().SetTimeFit(false);
  graph.horizontal_axis().SetRange(
      views::GraphRange{from, to, views::GraphRange::TIME});

  graph.UpdateData();
}

// Pumps the event loop until every line has the history it was asked to plot,
// and reports a failure if that never happens.
//
// This replaced a blind one-second pump. A fixed pump is a shared budget: a
// run capturing many windows settles each one less than a single-window run
// does, so the same spec rendered differently depending on the size of the
// `--only` list, and nothing failed when it came up short — the incomplete
// image just shipped. Waiting on the lines themselves makes the capture
// independent of what else the run contained.
//
// It does not on its own fix a blank trend: the line can hold a full series
// whose points all sit outside the visible window (see the interval cap in
// LocalHistoryService::ReadRaw). Hence the failure path — a graph capture
// that would be blank must fail the run rather than ship.
bool WaitForGraphSeries(MetrixGraph& graph, std::chrono::milliseconds timeout) {
  auto lines_pending = [&graph] {
    int pending = 0;
    for (auto* pane : graph.panes()) {
      for (auto* line : pane->plot().lines()) {
        MetrixDataSource& source =
            static_cast<MetrixGraph::MetrixLine*>(line)->data_source();
        // `is_ready()` alone is not enough: a spec whose requested range is
        // still the default one reports ready with nothing fetched, which is
        // exactly the state the blind pump used to grab.
        if (!source.connected() || !source.is_ready() ||
            source.timed_data().values().empty()) {
          ++pending;
        }
      }
    }
    return pending;
  };

  QElapsedTimer elapsed;
  elapsed.start();
  int pending = lines_pending();
  while (pending > 0 && !elapsed.hasExpired(timeout.count())) {
    scada::screenshot_generator::PumpEventLoopFor(
        std::chrono::milliseconds(50));
    pending = lines_pending();
  }

  if (pending > 0) {
    ADD_FAILURE() << pending << " graph line(s) had no plotted history after "
                  << timeout.count() << " ms; the capture would be blank";
    return false;
  }
  return true;
}

}  // namespace

void SaveGraphScreenshot(const ScreenshotSpec& spec,
                         NodeService& node_service,
                         TimedDataService& timed_data_service,
                         const boost::json::value& json) {
  CapturePublishGuard publish_guard{spec.filename};

  const boost::json::object& jgraph = ResolveGraphConfig(spec, json);

  // Held until after the grab so the nodes stay resident (see the function).
  std::vector<TimedDataSpec> residency_pins =
      MakeGraphItemNodesResident(node_service, timed_data_service, jgraph);

  MetrixGraph graph{MetrixGraphContext{timed_data_service}};
  BuildGraphFromJson(graph, jgraph, json);

  for (auto* pane : graph.panes())
    static_cast<MetrixGraph::MetrixPane*>(pane)->ShowLegend(true);

  // Let the lines' async history reads and current-value subscribe/read chains
  // (routed through the executor + Qt event loop) settle so the plotted series,
  // the legend current/min/max/average cells and the cursor readout are
  // populated before the grab. A standalone graph doesn't get the incidental
  // pumping a full capture run accumulates, so it must pump for itself.
  WaitForGraphSeries(graph, std::chrono::seconds(30));

  // Drop a time cursor so the legend's "@ cursor" column is populated. Without
  // one the capture named graph-cursor.png showed no cursor at all and every
  // row read "—", so the readout shipped unvalidated. Placed at two thirds of
  // the displayed range: inside the data, clear of the legend overlay at the
  // pane's top-left.
  //
  // Opt-in per graph configuration: a capture whose subject is something else
  // (the limit bands) draws a cursor line and a cursor time label across its
  // picture for no reason, and a cursor in every graph capture is half of why
  // the two of them used to render identically.
  const auto* jcursor = jgraph.if_contains("cursor");
  const GraphRange range = graph.horizontal_axis().range();
  if (jcursor && jcursor->as_bool() && range.low() < range.high()) {
    const GraphCursor& cursor = graph.horizontal_axis().AddCursor(
        range.low() + (range.high() - range.low()) * 2.0 / 3.0);
    graph.SelectCursor(&cursor);
    scada::screenshot_generator::PumpEventLoopFor(
        std::chrono::milliseconds(100));
  }

  // Render — matches graph_qt's RenderWidget pattern exactly.
  graph.setFixedSize(spec.width, spec.height);
  graph.show();
  scada::screenshot_generator::PumpEventLoopFor(std::chrono::milliseconds(200));

  QPixmap pixmap = graph.grab();
  auto output_path = GetOutputDir() / spec.filename;
  if (!publish_guard.ShouldPublish())
    return;

  pixmap.save(QString::fromStdString(output_path.string()));
}

void SaveSeriesInspectorScreenshot(const ScreenshotSpec& spec,
                                   NodeService& node_service,
                                   TimedDataService& timed_data_service,
                                   const boost::json::value& json) {
  const boost::json::object& jgraph = ResolveGraphConfig(spec, json);

  // Held until after the grab so the nodes stay resident (see the function).
  std::vector<TimedDataSpec> residency_pins =
      MakeGraphItemNodesResident(node_service, timed_data_service, jgraph);

  MetrixGraph graph{MetrixGraphContext{timed_data_service}};
  BuildGraphFromJson(graph, jgraph, json);

  // Let the async history/current-value chains settle so the readout and the
  // quality pill are populated before the grab (see SaveGraphScreenshot).
  WaitForGraphSeries(graph, std::chrono::seconds(30));

  // Point at the first pane's series (the first graphed item)
  // deterministically: NewPane() auto-selects the last-created pane, so
  // graph.primary_line() would otherwise follow the bottom pane rather than the
  // top one.
  MetrixGraph::MetrixLine* line =
      graph.panes().empty()
          ? nullptr
          : static_cast<MetrixGraph::MetrixPane*>(graph.panes().front())
                ->primary_line();

  // The series' presentation is the shell Inspector's since 2026-08-30 — the
  // Graph tab no longer carries an inspector of its own. So this renders the
  // Inspector as a chart selection fills it: the element card from the series'
  // own live spec, through the same SelectionModel the shell routes, plus the
  // series section underneath.
  //
  // No `load_limits` is wired and the Measurements bands still render:
  // MakeGraphItemNodesResident above already made the graphed node resident,
  // property children included, so MakeLimitRows reads them straight off the
  // node. That fetch handler is for a selection the shell made from a tree,
  // which never makes them resident on its own.
  InspectorPanel panel{InspectorPanelContext{}};
  SelectionModel selection{SelectionModelContext{timed_data_service}};
  if (line) {
    selection.SelectTimedData(line->data_source().timed_data());
    panel.ShowSelection(selection);
    panel.ShowSeries(
        InspectorSeriesView{.color = line->color(),
                            .own_pane = line->plot().lines().size() == 1,
                            .dots = line->dots_shown(),
                            .stepped = line->stepped()});
  }
  SaveScreenshot(&panel, spec);
}
