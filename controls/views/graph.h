#pragma once

#include "controls/types.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/graph/graph.h"
#include "ui/views/controls/graph/graph_line.h"
#include "ui/views/controls/graph/graph_pane.h"
#include "ui/views/controls/graph/graph_plot.h"
#include "ui/views/controls/graph/graph_widget.h"

using GraphLine = views::GraphLine;
using GraphPane = views::GraphPane;
using GraphWidget = views::GraphWidget;

class Graph : private views::ContextMenuController, public views::Graph {
 public:
  void SetContextMenuHandler(ContextMenuHandler handler) {
    context_menu_handler_ = handler;
    set_context_menu_controller(this);
  }

 private:
  // views::ContextMenuController
  virtual void ShowContextMenuForView(views::View* source,
                                      const gfx::Point& point) override {
    if (context_menu_handler_)
      context_menu_handler_(point);
  }

  ContextMenuHandler context_menu_handler_;
};
