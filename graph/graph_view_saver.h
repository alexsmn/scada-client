#pragma once

#include "base/time_utils.h"

struct GraphViewSaver {
  void Save() {
    base::Time time =
        base::Time::FromDoubleT(graph_.horizontal_axis().range().high());
    base::TimeDelta span =
        TimeDeltaFromSecondsF(graph_.horizontal_axis().range().delta());

    // time scale
    {
      WindowItem& item = definition_.AddItem("TimeScale");
      item.SetString("time", graph_.horizontal_axis().time_fit()
                                 ? std::string("Now")
                                 : SerializeToString(time));
      item.SetString("span", SerializeToString(span));
      item.SetBool("scrollBar", graph_.horizontal_scroll_bar_visible());
    }

    // value scale
    // WindowItem& item = def.AddItem(_T("ValueScale"));
    // panes
    for (auto* pane : graph_.panes()) {
      SavePane(*pane);
    }

    SaveTimeRange(definition_, graph_view_.GetTimeRange());

    definition_.AddItem("Graph").SetString(
        "bk_color",
        aui::ColorToString(graph_.palette().color(graph_.backgroundRole())));
  }

  void SavePane(const GraphPane& pane) {
    {
      WindowItem& item = definition_.AddItem("GraphPane");
      item.SetInt("ix", pane_ix);
      item.SetInt("size", pane.size_percent_);
    }

    for (auto* line : pane.plot().lines()) {
      SaveLine(*static_cast<const MetrixGraph::MetrixLine*>(line));
    }

    pane_ix++;
  }

  void SaveLine(const MetrixGraph::MetrixLine& line) {
    WindowItem& item = definition_.AddItem("Item");
    item.SetInt("pane", pane_ix);
    item.SetString("path", line.data_source().GetPath());
    item.SetString("clr", aui::ColorToString(line.color()));
    item.SetInt("dots", line.dots_shown() ? 1 : 0);
    item.SetInt("stepped", line.stepped() ? 1 : 0);
  }

  const MetrixGraph& graph_;
  const GraphView& graph_view_;
  WindowDefinition& definition_;

  int pane_ix = 1;
};