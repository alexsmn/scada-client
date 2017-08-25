#include "components/graph/metrix_graph.h"

#include <algorithm>

#include "base/strings/stringprintf.h"
#include "base/win/scoped_handle.h"
#include "base/format_time.h"
#include "base/minute_time.h"
#include "components/graph/metrix_data_source.h"
#include "core/monitored_item_service.h"

#if defined(UI_QT)
#include <qpainter.h>
#include "graph_qt/graph_axis.h"
#include "graph_qt/graph_cursor.h"
#elif defined(UI_VIEWS)
#include "ui/views/controls/graph/graph_axis.h"
#include "ui/views/controls/graph/graph_cursor.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font.h"
#include "skia/ext/skia_utils_win.h"
#endif

template<class TimedVQMap, class Iter, typename Arg>
void GetDrawRange(TimedVQMap& values, Arg x1, Arg x2, Iter& begin, Iter& end) {
  begin = end = values.begin();
  while (end != values.end()) {
    const Arg& x = end->first;
    if (x <= x1)
      begin = end;	// start from point previous to first visible
    else if (x >= x2) {
      end++;			// end by first invisible point
      break;
    }
    end++;
  }
}

// MetrixGraph::MetrixPane

void MetrixGraph::MetrixPane::UpdateLegend() {
  if (legend_.get()) {
#if defined(UI_VIEWS)
    gfx::Size size = legend_->GetPreferredSize();
    legend_->SetBoundsRect(gfx::Rect(legend_->location(), size));
#endif
  }
}

void MetrixGraph::MetrixPane::ShowLegend(bool show) {
  if (show) {
    if (legend_.get())
      return;

    legend_.reset(new Legend(*this));
    plot().AddWidget(*legend_);

#if defined(UI_VIEWS)
    gfx::Size size = legend_->GetPreferredSize();
    legend_->SetBoundsRect(gfx::Rect(legend_->location(), size));
#endif

  } else {
    if (!legend_.get())
      return;

    plot().RemoveWidget(*legend_);
    legend_.reset();
  }
}

// MetrixGraph::Legend

#if defined(UI_QT)
void MetrixGraph::Legend::paintEvent(QPaintEvent* e) {
  QPainter painter(this);

  MetrixGraph& graph = pane().graph();

  //	dc.Rectangle(rect.left, rect.top, rect.right + 1, rect.bottom + 1);

  int y = GRAPH_LEG_MARGY;
  for (views::GraphPlot::Lines::const_iterator i = plot().lines().begin();
                                               i != plot().lines().end(); i++) {
    MetrixLine& line = static_cast<MetrixLine&>(**i);

    QRect box(GRAPH_LEG_MARGX, y + 3,
              GRAPH_LEG_ROW - 6, GRAPH_LEG_ROW - 6);

    //		CBrush brush;
    //		brush.CreateSolidBrush(line.color);
    //		dc.FillRect(&box, brush);

    scada::DataValue value;
    const views::GraphCursor* cursor = graph.selected_cursor();
    if (cursor && cursor->axis_->is_vertical()) {
      base::Time cursor_time = base::Time::FromDoubleT(cursor->position_);
      value = line.data_source().timed_data().GetValueAt(cursor_time);
    } else {
      value = line.data_source().timed_data().current();
    }

    // Draw title.
    auto title = QString::fromStdWString(line.data_source().title());
    int p = box.x() + box.width() + 3;
    QRect rc(p, y, std::max(0, title_width_ - p), GRAPH_LEG_ROW);
    // TODO: End ellipsis.
    painter.drawText(rc, Qt::AlignLeft | Qt::AlignVCenter, title);

    // Draw value.
    auto text = QStringLiteral("= ") + QString::fromStdWString(line.data_source().timed_data().GetValueString(value.value, value.qualifier));
    rc = QRect(title_width_, y, 80, GRAPH_LEG_ROW);
    // TODO: End ellipsis.
    painter.drawText(rc, Qt::AlignLeft | Qt::AlignVCenter, text);

    // Draw time.
    auto time_text = QString::fromStdString(FormatTime(value.time));
    p = title_width_ + 80;
    rc = QRect(p, y, std::max(0, width() - GRAPH_LEG_MARGX - p), GRAPH_LEG_ROW);
    // TODO: End ellipsis.
    painter.drawText(rc, Qt::AlignLeft | Qt::AlignVCenter, time_text);

    y += GRAPH_LEG_ROW;
  }
}
#elif defined(UI_VIEWS)
void MetrixGraph::Legend::OnPaint(gfx::Canvas* canvas) {
  MetrixGraph& graph = pane().graph();

  //	dc.Rectangle(rect.left, rect.top, rect.right + 1, rect.bottom + 1);

  int y = GRAPH_LEG_MARGY;
  for (views::GraphPlot::Lines::const_iterator i = plot().lines().begin();
                                               i != plot().lines().end(); i++) {
    MetrixLine& line = static_cast<MetrixLine&>(**i);

    gfx::Rect box(GRAPH_LEG_MARGX, y + 3,
                  GRAPH_LEG_ROW - 6, GRAPH_LEG_ROW - 6);

    //		CBrush brush;
    //		brush.CreateSolidBrush(line.color);
    //		dc.FillRect(&box, brush);

    scada::DataValue value;
    const views::GraphCursor* cursor = graph.selected_cursor();
    if (cursor && cursor->axis_->is_vertical()) {
      base::Time cursor_time = base::Time::FromDoubleT(cursor->position_);
      value = line.data_source().timed_data().GetValueAt(cursor_time);
    } else {
      value = line.data_source().timed_data().current();
    }

    // Draw title.
    const auto& title = line.data_source().title();
    int p = box.x() + box.width() + 3;
    gfx::Rect rc(p, y, std::max(0, title_width_ - p), GRAPH_LEG_ROW);
    canvas->DrawString(title, graph.font_, SK_ColorBLACK, rc,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Draw value.
    base::string16 text = L"= " + line.data_source().timed_data().GetValueString(value.value, value.qualifier);
    rc = gfx::Rect(title_width_, y, 80, GRAPH_LEG_ROW);
    canvas->DrawString(text, graph.font_, SK_ColorBLACK, rc,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Draw time.
    std::string time_text = FormatTime(value.time);
    p = title_width_ + 80;
    rc = gfx::Rect(p, y, std::max(0, width() - GRAPH_LEG_MARGX - p), GRAPH_LEG_ROW);
    canvas->DrawString(time_text, graph.font_, SK_ColorBLACK, rc,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    y += GRAPH_LEG_ROW;
  }
}
#endif

#if defined(UI_QT)
QSize MetrixGraph::Legend::sizeHint() const {
  MetrixGraph& graph = pane().graph();
  
  int width = 30;

  for (views::GraphPlot::Lines::const_iterator i = plot().lines().begin();
                                               i != plot().lines().end(); ++i) {
    MetrixLine& line = static_cast<MetrixLine&>(**i);
    auto title = QString::fromStdWString(line.data_source().title());
    auto size = QFontMetrics(font()).width(title);
    if (size > width)
      width = size;
  }
  width += GRAPH_LEG_MARGX * 2 + GRAPH_LEG_ROW - 6 + 5;
  int height = plot().lines().empty() ? GRAPH_LEG_ROW :
                                        GRAPH_LEG_ROW * plot().lines().size();
  height += 2 * GRAPH_LEG_MARGY;

  const_cast<Legend*>(this)->title_width_ = width;
  return QSize(width + 80 + 150, height);
}
#elif defined(UI_VIEWS)
gfx::Size MetrixGraph::Legend::GetPreferredSize() const {
  MetrixGraph& graph = pane().graph();
  
  int width = 30;

  for (views::GraphPlot::Lines::const_iterator i = plot().lines().begin();
                                               i != plot().lines().end(); ++i) {
    MetrixLine& line = static_cast<MetrixLine&>(**i);
    const auto& title = line.data_source().title();
    gfx::Size size = gfx::Canvas::GetStringSize(title, graph.font_);
    if (size.width() > width)
      width = size.width();
  }
  width += GRAPH_LEG_MARGX * 2 + GRAPH_LEG_ROW - 6 + 5;
  int height = plot().lines().empty() ? GRAPH_LEG_ROW :
                                        GRAPH_LEG_ROW * plot().lines().size();
  height += 2 * GRAPH_LEG_MARGY;

  const_cast<Legend*>(this)->title_width_ = width;
  return gfx::Size(width + 80 + 150, height);
}
#endif

// MetrixGraph::MetrixLine

MetrixGraph::MetrixLine::MetrixLine()
    : data_source_(new MetrixDataSource) {
  SetDataSource(data_source_.get());
  set_auto_range(false);
}

MetrixGraph::MetrixLine::~MetrixLine() {
  SetDataSource(nullptr);
}

void MetrixGraph::MetrixLine::OnDataSourceHistoryChanged() {
  views::GraphLine::OnDataSourceHistoryChanged();

  const rt::TimedDataSpec& timed_data = data_source_->timed_data();

  assert(timed_data.historical());

  const rt::TimedVQMap* values = timed_data.values();  
  if (!values || values->empty())
    return;

  // Scroll to last time.
  base::Time last_time = timed_data.current().time;
  if (last_time.is_null())
    return;

  double time = last_time.ToDoubleT();
  if (graph().right_range_limit_ != std::numeric_limits<double>::max() &&
      graph().right_range_limit_ < time) {
    graph().right_range_limit_ = time;
  }
  graph().Fit();
}

void MetrixGraph::MetrixLine::OnDataSourceCurrentValueChanged() {
  views::GraphLine::OnDataSourceCurrentValueChanged();

  if (!graph().selected_cursor())
    graph().UpdateCurBox();
}

void MetrixGraph::MetrixLine::OnDataSourceItemChanged() {
  views::GraphLine::OnDataSourceItemChanged();

  pane().UpdateLegend();

  if (graph().controller())
    graph().controller()->OnLineItemChanged(*this);
}

void MetrixGraph::MetrixLine::OnDataSourceDeleted() {
  MetrixGraph& graph = this->graph();
  plot().DeleteLine(*this);

  if (graph.controller())
    graph.controller()->OnGraphModified();
}

// MetrixGraph

MetrixGraph::MetrixGraph(TimedDataService& timed_data_service)
    : timed_data_service_(timed_data_service),
      monitored_item_service_(NULL) {
  update_data_timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(50),
      this, &MetrixGraph::UpdateData);
}

void MetrixGraph::UpdateCurBox() {
  // update box for specified position of cursor line
  for (Panes::const_iterator i = panes().begin(); i != panes().end(); i++) {
    MetrixPane& pane = *static_cast<MetrixPane*>(*i);
    if (pane.plot().lines().empty())
      continue;

    if (pane.legend_.get()) {
#if defined(UI_QT)
      pane.legend_->update();
#elif defined(UI_VIEWS)
      pane.legend_->SchedulePaint();
#endif
    }
  }
}

void MetrixGraph::MetrixLine::UpdateTimeRange() {
  if (data_source().connected())
    data_source().SetFrom(base::Time::FromDoubleT(graph().horizontal_axis().range().low()));
}

void MetrixGraph::UpdateData() {
  for (Panes::const_iterator i = panes().begin(); i != panes().end(); ++i) {
    views::GraphPane& pane = **i;
    const views::GraphPlot::Lines& lines = pane.plot().lines();
    for (views::GraphPlot::Lines::const_iterator i = lines.begin();
                                                 i != lines.end(); ++i) {
      static_cast<MetrixLine*>(*i)->UpdateTimeRange();
    }
  }
}

MetrixGraph::MetrixLine& MetrixGraph::NewLine(const std::string& path, MetrixPane& pane) {
  rt::TimedDataSpec spec;
  try {
    spec.Connect(timed_data_service_, path);
  } catch (const std::exception& /*err*/) {
  }

  MetrixLine* line = new MetrixLine();
  pane.plot().AddLine(*line);
  
  line->data_source().SetTimedData(spec);

  return *line;
}

MetrixGraph::MetrixPane& MetrixGraph::NewPane() {
  MetrixPane& pane = * new MetrixPane();

  // TODO: fix this
  /*if (selected_pane_) {
  pane->legend.show = selected_pane_->legend.show;
  pane->cur_box.show = selected_pane_->cur_box.show;
  }*/

  AddPane(pane);

  SelectPane(&pane);

  return pane;
}
