#pragma once

#include <memory>

#include "base/timer/timer.h"
#include "components/graph/metrix_data_source.h"
#include "timed_data/timed_data.h"
#include "timed_data/timed_data_delegate.h"

#if defined(UI_QT)
#include "graph_qt/graph.h"
#include "graph_qt/graph_line.h"
#include "graph_qt/graph_pane.h"
#include "graph_qt/graph_plot.h"
#include "graph_qt/graph_widget.h"
#elif defined(UI_VIEWS)
#include "ui/views/controls/graph/graph.h"
#include "ui/views/controls/graph/graph_line.h"
#include "ui/views/controls/graph/graph_pane.h"
#include "ui/views/controls/graph/graph_plot.h"
#include "ui/views/controls/graph/graph_widget.h"
#endif

namespace scada {
class MonitoredItemService;
}

class MetrixGraph : public views::Graph {
 public:
//  typedef std::map<long, Bar> BarMap;

  class MetrixPane;
  class MetrixLine;

  class MetrixWidget : public views::GraphWidget {
  public:
    explicit MetrixWidget(MetrixPane& pane)
        : GraphWidget(pane) {
    }

    MetrixPane& pane() const { return static_cast<MetrixPane&>(pane_); }
  };

  class Legend : public MetrixWidget {
  public:
    explicit Legend(MetrixPane& pane) : MetrixWidget(pane) { }

#if defined(UI_QT)
    // QWidget
    virtual void paintEvent(QPaintEvent* e) override;
    virtual QSize sizeHint() const override;
#elif defined(UI_VIEWS)
    // GraphWidget
    virtual void OnPaint(gfx::Canvas* canvas);
    virtual gfx::Size GetPreferredSize() const;
#endif

  private:
    static const int GRAPH_LEG_MARGX	= 5;	// margin inside legend
    static const int GRAPH_LEG_MARGY	= 2;	// margin inside legend
    static const int GRAPH_LEG_ROW		= 13;	// legend row height

    int title_width_;
  };
  
  class MetrixPane : public views::GraphPane {
   public:
    MetrixGraph& graph() const {
      return static_cast<MetrixGraph&>(GraphPane::graph());
    }
    MetrixLine* primary_line() const {
      return static_cast<MetrixLine*>(plot().primary_line());
    }

    bool show_legend() const { return legend_.get() != NULL; }
    void UpdateLegend();
    void ShowLegend(bool show);

   private:
    friend class MetrixGraph;

    std::unique_ptr<Legend> legend_;
  };

 
  class MetrixLine : public views::GraphLine {
   public:
    MetrixLine();
    virtual ~MetrixLine();

    MetrixPane& pane() const { return static_cast<MetrixPane&>(GraphLine::plot().pane()); }
    MetrixGraph& graph() const { return pane().graph(); }

    MetrixDataSource& data_source() { return *data_source_; }
    const MetrixDataSource& data_source() const { return *data_source_; }

    void UpdateTimeRange();
    
    std::unique_ptr<MetrixDataSource> data_source_;

   protected:
    // GraphDataSource::Observer
    virtual void OnDataSourceHistoryChanged() override;
    virtual void OnDataSourceCurrentValueChanged() override;
    virtual void OnDataSourceItemChanged() override;
    virtual void OnDataSourceDeleted() override;
  };

  explicit MetrixGraph(TimedDataService& timed_data_service);

  MetrixPane* selected_pane() const { return static_cast<MetrixPane*>(Graph::selected_pane()); }
  MetrixLine* primary_line() const {
    MetrixPane* pane = selected_pane();
    return pane ? pane->primary_line() : NULL;
  }
  
  MetrixLine& NewLine(const std::string& path, MetrixPane& pane);
  MetrixPane& NewPane();

  void UpdateData();
  
  // Graph
  virtual void UpdateCurBox() override;

 private:
  TimedDataService& timed_data_service_;

  scada::MonitoredItemService* monitored_item_service_;
  
  base::RepeatingTimer update_data_timer_;
};