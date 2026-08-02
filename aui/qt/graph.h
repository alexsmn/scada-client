#pragma once

#include "aui/handlers.h"
#include "graph_qt/graph.h"
#include "graph_qt/graph_axis.h"
#include "graph_qt/graph_cursor.h"
#include "graph_qt/graph_line.h"
#include "graph_qt/graph_pane.h"
#include "graph_qt/graph_plot.h"
#include "graph_qt/graph_widget.h"
#include "graph_qt/model/graph_data_source.h"
#include "graph_qt/model/graph_range.h"
#include "graph_qt/model/graph_types.h"

// The AUI graph abstraction re-exports the Qt graph (graph_qt submodule) so the
// client depends only on this facade, never on graph_qt directly. graph_qt is a
// declared dependency of aui; keeping the client off it is what lets aui be
// extracted as a standalone repo (client/CLAUDE.md rule 13). When the client
// needs another graph_qt symbol, add the include + alias here first.
using GraphAxis = views::GraphAxis;
using GraphCursor = views::GraphCursor;
using GraphDataSource = views::GraphDataSource;
using GraphLine = views::GraphLine;
using GraphPane = views::GraphPane;
using GraphPlot = views::GraphPlot;
using GraphPoint = views::GraphPoint;
using GraphRange = views::GraphRange;
using GraphValue = views::GraphValue;
using GraphWidget = views::GraphWidget;
using PointEnumerator = views::PointEnumerator;
using views::kGraphUnknownValue;

class Graph : public views::Graph {
 public:
  void SetContextMenuHandler(ContextMenuHandler handler) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested,
            [this, handler](const QPoint& pos) { handler(mapToGlobal(pos)); });
  }
};
