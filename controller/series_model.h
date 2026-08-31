#pragma once

#include "aui/color.h"

#include <functional>

// The presentation of the one series a view currently has configurable — the
// colour it is plotted in and the display flags it carries.
//
// It exists so the shell Inspector can show a plotted series without knowing
// what a graph is: `Controller::GetSeriesModel()` hands it over the same way
// `GetSelectionModel()` and `GetTimeModel()` hand over their models, and the
// Inspector renders whatever it is given. Before 2026-08-30 the Graph view
// carried a second inspector panel of its own inside the tab, so the two sat
// side by side naming the same series — see the trend screens, which draw one
// right-hand column.
//
// Everything else the old panel showed — identity, limit bands, node and
// quality — the Inspector already renders from the selection, so it is
// deliberately absent here: this model is the part that is not derivable from
// the selected node.
class SeriesModel {
 public:
  virtual ~SeriesModel() = default;

  // Whether a series is configurable at all. False leaves the Inspector's
  // series section hidden — an empty graph has nothing to say about colour.
  virtual bool HasSeries() const = 0;

  // The colour the series is plotted in, and the write that recolours it.
  // SetColor is the one field of this model that writes, matching the web
  // trend's inspector.
  virtual scada::aui::Color GetSeriesColor() const = 0;
  virtual void SetSeriesColor(scada::aui::Color color) = 0;

  // Display flags, read-only here: the operator toggles them through the
  // view's own commands (ID_GRAPH_DOTS, ID_GRAPH_STEPS), and the Inspector
  // reports what they currently are.
  virtual bool IsSeriesOnOwnPane() const = 0;
  virtual bool AreSeriesDotsShown() const = 0;
  virtual bool IsSeriesStepped() const = 0;

  // Invoked when the series or its presentation changed, so the host can
  // re-read this model. Shaped like `SelectionModel::change_handler` and owned
  // by the host, which sets it while the view is active and clears it after —
  // a flag toggled from the menu changes no selection, so without this the
  // Inspector would go stale.
  using ChangeHandler = std::function<void()>;
  ChangeHandler change_handler;
};
