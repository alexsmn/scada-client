#pragma once

#include "base/string_piece_util.h"
#include "base/strings/string_util.h"
#include "base/time_utils.h"
#include "services/profile.h"
#include "window_definition_util.h"

struct GraphViewLoader {
  void Read() {
    for (auto& item : definition_.items) {
      if (item.name_is("GraphPane")) {
        ReadPane(item);

      } else if (item.name_is("Item")) {
        ReadItem(item);

      } else if (item.name_is("TimeScale")) {
        ReadTimeScale(item);
      }
    }

    if (!time_set) {
      FixTimeRange();
    }
  }

  void ReadPane(const WindowItem& item) {
    views::GraphPane* pane = &graph_.NewPane();

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
                                      : aui::StringToColor(color_string);
    // add line
    MetrixGraph::MetrixLine& line =
        graph_.NewLine(path, *static_cast<MetrixGraph::MetrixPane*>(pane));
    line.SetColor(color.native_color());
    line.set_dots_shown(dots);
    line.set_stepped(stepped);
  }

  void ReadTimeScale(const WindowItem& item) {
    auto srange = item.GetString("span");
    auto stime = item.GetString("time");
    base::Time from, to;
    graph_.m_time_fit =
        base::EqualsCaseInsensitiveASCII(AsStringPiece(stime), "Now");
    if (graph_.m_time_fit || !Deserialize(stime, to)) {
      graph_.m_time_fit = true;
      to = base::Time::Now();
    }
    base::TimeDelta span = base::TimeDelta::FromHours(1);
    Deserialize(srange, span);
    from = to - span;
    graph_.horizontal_axis().SetRange(views::GraphRange(
        from.ToDoubleT(), to.ToDoubleT(), views::GraphRange::TIME));
    time_set = true;
  }

  void FixTimeRange() {
    if (auto time_range = RestoreTimeRange(definition_)) {
      graph_.m_time_fit = time_range->type != TimeRange::Type::Custom;
      auto [start, end] = GetTimeRangeBounds(*time_range);
      graph_.horizontal_axis().SetRange(views::GraphRange(
          start.ToDoubleT(), end.ToDoubleT(), views::GraphRange::TIME));
    } else {
      base::Time now = base::Time::Now();
      graph_.horizontal_axis().SetRange(views::GraphRange(
          (now - profile_.graph_view.default_span).ToDoubleT(), now.ToDoubleT(),
          views::GraphRange::TIME));
    }
  }

  const WindowDefinition& definition_;
  const Profile& profile_;
  MetrixGraph& graph_;
  GraphView& graph_view_;

  using PaneMap = std::unordered_map<int, views::GraphPane*>;
  PaneMap pane_map;

  bool time_set = false;

  static const size_t kMaxPanes = 10;
};
