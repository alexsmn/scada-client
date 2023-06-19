#pragma once

#include <memory>

#include "base/timer/timer.h"
#include "components/graph/metrix_data_source.h"
#include "controls/graph.h"
#include "timed_data/timed_data.h"
#include "timed_data/timed_data_delegate.h"

class TimedDataService;

struct MetrixGraphContext {
  TimedDataService& timed_data_service_;
};

class MetrixGraph : private MetrixGraphContext, public Graph {
 public:
  //  typedef std::map<long, Bar> BarMap;

  class MetrixPane;
  class MetrixLine;

  class MetrixWidget : public GraphWidget {
   public:
    explicit MetrixWidget(MetrixPane& pane) : GraphWidget(pane) {}

    MetrixPane& pane() const { return static_cast<MetrixPane&>(pane_); }
  };

  class Legend : public MetrixWidget {
   public:
    explicit Legend(MetrixPane& pane);

    void Update();

#if defined(UI_QT)
    // QWidget
    virtual void paintEvent(QPaintEvent* e) override;
    virtual QSize sizeHint() const override;
#endif

   private:
    scada::DataValue GetCurrentValue(const MetrixDataSource& data_source) const;
    std::u16string GetText(const MetrixDataSource& data_source,
                           int column_id) const;
    int GetColumnWidth(int column_id) const;
    int GetColumnCount() const;

    static const int MARGX = 5;    // margin inside legend
    static const int MARGY = 5;    // margin inside legend
    static const int INDENTX = 2;  // distance between columns
    static const int ROW = 13;     // legend row height

    mutable int title_width_ = 0;
  };

  class MetrixPane : public GraphPane {
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

  class MetrixLine : public GraphLine {
   public:
    MetrixLine();
    virtual ~MetrixLine();

    MetrixPane& pane() const {
      return static_cast<MetrixPane&>(GraphLine::plot().pane());
    }
    MetrixGraph& graph() const { return pane().graph(); }

    MetrixDataSource& data_source() { return *data_source_; }
    const MetrixDataSource& data_source() const { return *data_source_; }

    void UpdateTimeRange();

    std::unique_ptr<MetrixDataSource> data_source_;

   protected:
    // MetrixDataSource::Observer
    virtual void OnDataSourceHistoryChanged() override;
    virtual void OnDataSourceCurrentValueChanged() override;
    virtual void OnDataSourceItemChanged() override;
    virtual void OnDataSourceDeleted() override;
  };

  explicit MetrixGraph(MetrixGraphContext&& context);

  MetrixPane* selected_pane() const {
    return static_cast<MetrixPane*>(Graph::selected_pane());
  }
  MetrixLine* primary_line() const {
    MetrixPane* pane = selected_pane();
    return pane ? pane->primary_line() : NULL;
  }

  MetrixLine& NewLine(std::string_view path, MetrixPane& pane);
  MetrixPane& NewPane();

  void Fit();

  void UpdateData();

  // Graph
  virtual void UpdateCurBox() override;
  virtual QString GetXAxisLabel(double val) const override;

  bool m_time_fit = true;

 private:
  base::RepeatingTimer update_data_timer_;
};
