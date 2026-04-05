#pragma once

#include "aui/handlers.h"
#include "graph_qt/graph.h"
#include "graph_qt/graph_axis.h"
#include "graph_qt/graph_line.h"
#include "graph_qt/graph_pane.h"
#include "graph_qt/graph_plot.h"
#include "graph_qt/graph_widget.h"

using GraphLine = views::GraphLine;
using GraphPane = views::GraphPane;
using GraphWidget = views::GraphWidget;

class Graph : public views::Graph {
 public:
  void SetContextMenuHandler(ContextMenuHandler handler) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested,
            [this, handler](const QPoint& pos) { handler(mapToGlobal(pos)); });
  }
};
