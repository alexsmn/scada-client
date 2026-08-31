#pragma once

#include "aui/graph.h"
#include "base/lifetime.h"
#include "graph/metrix_data_source.h"

#include "timed_data/timed_data.h"
#include "timed_data/timed_data_observer.h"
#include <QTimer>

#include <memory>

class TimedDataService;

struct MetrixGraphContext {
  TimedDataService& timed_data_service_;
};

class MetrixGraph : private MetrixGraphContext, public Graph {
 public:
  void UpdateCurBox();

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

#if defined(UI_QT)
    // The value-grid readout: per-series swatch + name and
    // current/min/max/average/at-cursor columns (see trend.html). `ThemedSize`
    // is the size that layout requires for it.
    void PaintThemed(QPainter& painter) const;
    QSize ThemedSize() const;
    // Formats the value at the selected time cursor for `data_source`, or the
    // empty-cell placeholder when no horizontal cursor is set.
    QString ValueAtCursorText(const MetrixDataSource& data_source) const;
#endif

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

    MetrixDataSource& data_source() SCADA_LIFETIME_BOUND {
      return *data_source_;
    }
    const MetrixDataSource& data_source() const SCADA_LIFETIME_BOUND {
      return *data_source_;
    }

    void UpdateTimeRange();

    std::unique_ptr<MetrixDataSource> data_source_;

   protected:
    // MetrixDataSource::Observer
    virtual void OnDataSourceCurrentValueChanged() override;
    virtual void OnDataSourceItemChanged() override;
    virtual void OnDataSourceDeleted() override;

   private:
    // Recomputes the per-band limit marker styles from the data source's limits
    // and the active severity theme, then pushes them to the base GraphLine.
    void UpdateLimitStyles();
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

  void UpdateData();

#if defined(UI_QT)
  // Pins the plot canvas to an operator-chosen colour. Unlike the default
  // canvas — which is QPalette::Base and therefore follows the host OS
  // appearance — a colour set here survives both an OS light/dark switch and
  // the theme tokens, because it was asked for deliberately.
  void SetCanvasColor(const QColor& color);

  // Whether SetCanvasColor() pinned the canvas. Only an overridden canvas is
  // worth persisting into the window definition; a canvas that merely follows
  // the palette must not be written out, or it would freeze the appearance the
  // profile happened to be saved under.
  bool canvas_color_overridden() const { return canvas_color_overridden_; }

  // QWidget
  virtual void changeEvent(QEvent* event) override;
#endif

 private:
#if defined(UI_QT)
  // Applies the explicit theme's canvas colour, if a theme override is active
  // and the operator has not pinned a colour of their own. A no-op when the
  // canvas already carries the right colour, so it is safe to call from
  // changeEvent().
  void ApplyChartPalette();

  bool canvas_color_overridden_ = false;
#endif

  QTimer update_data_timer_;
};
