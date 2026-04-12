#include "graph_capture.h"

#include "screenshot_config.h"
#include "screenshot_output.h"

#include "base/time_utils.h"
#include "graph/metrix_graph.h"
#include "profile/window_definition.h"

#include <QApplication>
#include <QColor>
#include <QPixmap>
#include <QString>

#include <map>

WindowDefinition MakeGraphDefinition(const boost::json::value& json) {
  WindowDefinition def{"Graph"};
  const auto& graph = json.at("graph").as_object();

  for (const auto& jp : graph.at("panes").as_array()) {
    auto& item = def.AddItem("GraphPane");
    item.SetInt("ix", static_cast<int>(jp.at("index").as_int64()));
    item.SetInt("size", static_cast<int>(jp.at("size").as_int64()));
    if (auto* act = jp.as_object().if_contains("active");
        act && act->as_bool())
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

void SaveGraphScreenshot(const ScreenshotSpec& spec,
                         TimedDataService& timed_data_service,
                         const boost::json::value& json) {
  MetrixGraph graph{MetrixGraphContext{timed_data_service}};
  const auto& jgraph = json.at("graph").as_object();

  // Panes.
  std::map<int, MetrixGraph::MetrixPane*> pane_map;
  for (const auto& jp : jgraph.at("panes").as_array()) {
    auto& pane = graph.NewPane();
    int ix = static_cast<int>(jp.at("index").as_int64());
    pane.size_percent_ = static_cast<int>(jp.at("size").as_int64());
    pane_map[ix] = &pane;
    if (auto* act = jp.as_object().if_contains("active");
        act && act->as_bool())
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

  // Time range (span parsed from "HH:MM:SS").
  auto now = base::Time::Now();
  auto span_str = std::string(jgraph.at("time_scale").at("span").as_string());
  base::TimeDelta span;
  Deserialize(span_str, span);
  double from = (now - span).ToDoubleT();
  double to = now.ToDoubleT();
  graph.horizontal_axis().SetTimeFit(false);
  graph.horizontal_axis().SetRange(
      views::GraphRange{from, to, views::GraphRange::TIME});

  graph.UpdateData();

  for (auto* pane : graph.panes())
    static_cast<MetrixGraph::MetrixPane*>(pane)->ShowLegend(true);

  // Render — matches graph_qt's RenderWidget pattern exactly.
  graph.setFixedSize(spec.width, spec.height);
  graph.show();
  QApplication::processEvents();

  QPixmap pixmap = graph.grab();
  auto output_path = GetOutputDir() / spec.filename;
  pixmap.save(QString::fromStdString(output_path.string()));
}
