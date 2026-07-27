#pragma once

#include "base/time/time_wire_codec.h"

#include <chrono>

#include "base/time_utils.h"
#include "profile/profile.h"
#include "profile/window_definition_util.h"
#include <boost/algorithm/string/predicate.hpp>

struct GraphViewLoader {
  void Read() {
    for (auto& item : definition_.items) {
      if (item.name_is("GraphPane")) {
        ReadPane(item);
      } else if (item.name_is("Item")) {
        ReadItem(item);
      } else if (item.name_is("TimeScale")) {
        ReadTimeScale(item);
      } else if (item.name_is("Graph")) {
        ReadGraph(item);
      }
    }

    if (!time_scale_loaded_) {
      FixTimeRange();
      graph_.SetHorizontalScrollBarVisible(
          profile_.graph_view.default_scroll_bar);
    }
  }

  void ReadGraph(const WindowItem& item) {
    std::string_view color = item.GetString("bk_color");
    if (color.empty())
      return;

    // Profiles saved before the canvas followed QPalette::Base carry a
    // `bk_color` that was written unconditionally on every save, so a plain
    // white value is almost certainly the old hardwired canvas rather than a
    // deliberate choice — restoring it would pin every existing view to white
    // on a dark desktop. Drop it and let the palette decide; any other value
    // could only have come from the operator's own colour pick, so honour it.
    const scada::aui::Color loaded_color = scada::aui::StringToColor(color);
    if (loaded_color.rgba() == scada::aui::ColorCode::White)
      return;

    graph_view_.SetGraphColor(loaded_color);
  }

  void ReadPane(const WindowItem& item) {
    GraphPane* pane = &graph_.NewPane();

    pane->size_percent_ = item.GetInt("size", 100);

    int ix = item.GetInt("ix", -1);
    if (ix != -1)
      pane_map.try_emplace(ix, pane);

    if (item.GetInt("act", 0))
      graph_.SelectPane(pane);
  }

  void ReadItem(const WindowItem& item) {
    if (graph_.panes().size() >= kMaxPanes)
      return;

    auto path = item.GetString("path");
    auto stype = item.GetString("type", "GraphLine");
    auto color_string = item.GetString("clr");
    bool dots = item.GetInt("dots", 1) != 0;
    bool stepped = item.GetInt("stepped", 1) != 0;
    // pane
    int pane_ix = item.GetInt("pane", -1);
    auto i = pane_map.find(pane_ix);
    MetrixGraph::MetrixPane* pane = NULL;
    if (i != pane_map.end())
      pane = static_cast<MetrixGraph::MetrixPane*>(i->second);
    else
      pane = &static_cast<MetrixGraph::MetrixPane&>(graph_.NewPane());
    // make color
    auto color = color_string.empty() ? graph_view_.NewColor()
                                      : scada::aui::StringToColor(color_string);
    // add line
    MetrixGraph::MetrixLine& line =
        graph_.NewLine(path, *static_cast<MetrixGraph::MetrixPane*>(pane));
    line.SetColor(color.native_color());
    line.SetLineWeight(item.GetInt("width", profile_.graph_view.default_width));
    line.set_dots_shown(dots);
    line.set_stepped(stepped);
  }

  void ReadTimeScale(const WindowItem& item) {
    auto srange = item.GetString("span");
    auto stime = item.GetString("time");
    scada::Time from, to;
    bool time_fit = boost::iequals(stime, "Now");
    if (time_fit || !Deserialize(stime, to)) {
      time_fit = true;
      to = scada::Now();
    }
    scada::Duration span = std::chrono::hours{1};
    Deserialize(srange, span);
    from = to - span;
    graph_.horizontal_axis().SetRange(
        GraphRange(scada::base::EncodeDoubleT(from),
                   scada::base::EncodeDoubleT(to), GraphRange::TIME));
    graph_.SetHorizontalScrollBarVisible(
        item.GetBool("scrollBar", profile_.graph_view.default_scroll_bar));
    graph_.horizontal_axis().SetTimeFit(time_fit);
    time_scale_loaded_ = true;
  }

  void FixTimeRange() {
    if (auto time_range = RestoreTimeRange(definition_)) {
      auto [start, end] = scada::ToTimeRange(*time_range, now);
      graph_.horizontal_axis().SetRange(
          GraphRange{scada::base::EncodeDoubleT(start), scada::base::EncodeDoubleT(end), GraphRange::TIME});
      graph_.horizontal_axis().SetTimeFit(time_range->type !=
                                          scada::RelativeTimeRange::Type::Custom);
    } else {
      scada::Time now = scada::Now();
      graph_.horizontal_axis().SetRange(GraphRange(
          scada::base::EncodeDoubleT(now - profile_.graph_view.default_span),
          scada::base::EncodeDoubleT(now), GraphRange::TIME));
    }
  }

  const WindowDefinition& definition_;
  const Profile& profile_;
  MetrixGraph& graph_;
  GraphView& graph_view_;
  scada::Time now = scada::Now();

  using PaneMap = std::unordered_map<int, GraphPane*>;
  PaneMap pane_map;

  bool time_scale_loaded_ = false;

  static const size_t kMaxPanes = 10;
};
