#include "components/graph/metrix_graph.h"

#include <algorithm>

#include "base/format_time.h"
#include "base/minute_time.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/scoped_handle.h"
#include "components/graph/metrix_data_source.h"

#if defined(UI_QT)
#include <qpainter.h>
#include "graph_qt/graph_axis.h"
#include "graph_qt/graph_cursor.h"
#elif defined(UI_VIEWS)
#include "skia/ext/skia_utils_win.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font.h"
#include "ui/views/controls/graph/graph_axis.h"
#include "ui/views/controls/graph/graph_cursor.h"
#endif

namespace {

template <class TimedVQMap, class Iter, typename Arg>
void GetDrawRange(TimedVQMap& values, Arg x1, Arg x2, Iter& begin, Iter& end) {
  begin = end = values.begin();
  while (end != values.end()) {
    const Arg& x = end->first;
    if (x <= x1)
      begin = end;  // start from point previous to first visible
    else if (x >= x2) {
      end++;  // end by first invisible point
      break;
    }
    end++;
  }
}

int GetPercentReady(const TimedDataSpec& timed_data) {
  auto to = timed_data.current().source_timestamp;
  auto ready_from = timed_data.ready_from();
  auto requested_from = timed_data.from();

  if (ready_from <= requested_from)
    return 100;

  if (to.is_null() || ready_from.is_null() || requested_from.is_null())
    return 0;

  auto total = (to - requested_from).InMillisecondsF();
  auto ready = (to - ready_from).InMillisecondsF();

  if (total < 0 || ready < 0)
    return 0;

  if (total <= ready)
    return 100;

  if (std::abs(total) < std::numeric_limits<decltype(total)>::epsilon())
    return 1000;

  return ready / total * 100;
}

}  // namespace

// MetrixGraph::MetrixPane

void MetrixGraph::MetrixPane::UpdateLegend() {
  if (legend_)
    legend_->Update();
}

void MetrixGraph::MetrixPane::ShowLegend(bool show) {
  if (show) {
    if (legend_)
      return;

    legend_.reset(new Legend(*this));
    plot().AddWidget(*legend_);

    legend_->Update();

  } else {
    if (!legend_)
      return;

    plot().RemoveWidget(*legend_);
    legend_.reset();
  }
}

// MetrixGraph::Legend

MetrixGraph::Legend::Legend(MetrixPane& pane) : MetrixWidget(pane) {}

scada::DataValue MetrixGraph::Legend::GetCurrentValue(
    const MetrixDataSource& data_source) const {
  scada::DataValue value;
  const views::GraphCursor* cursor = graph().selected_cursor();
  if (cursor && cursor->axis_->is_vertical()) {
    base::Time cursor_time = base::Time::FromDoubleT(cursor->position_);
    return data_source.timed_data().GetValueAt(cursor_time);
  } else {
    return data_source.timed_data().current();
  }
}

std::wstring MetrixGraph::Legend::GetText(const MetrixDataSource& data_source,
                                            int column_id) const {
  switch (column_id) {
    case 0: {
      return data_source.title();
    }
    case 1: {
      auto data_value = GetCurrentValue(data_source);
      return L"= " + data_source.timed_data().GetValueString(
                         data_value.value, data_value.qualifier);
    }
    case 2: {
      auto data_value = GetCurrentValue(data_source);
      return base::ASCIIToUTF16(FormatTime(data_value.source_timestamp));
    }
    case 3: {
      auto percent = GetPercentReady(data_source.timed_data());
      return base::ASCIIToUTF16(std::to_string(percent) + '%');
    }
    default:
      return {};
  }
}

int MetrixGraph::Legend::GetColumnWidth(int column_id) const {
  switch (column_id) {
    case 0:
      return title_width_;
    case 1:
      return 80;
    case 2:
      return 150;
    case 3:
      return 50;
    default:
      return 0;
  }
}

int MetrixGraph::Legend::GetColumnCount() const {
  return 3;
}

void MetrixGraph::Legend::Update() {
#if defined(UI_VIEWS)
  gfx::Size size = GetPreferredSize();
  SetBoundsRect(gfx::Rect(location(), size));

#elif defined(UI_QT)
  title_width_ = 0;
  for (auto i = plot().lines().begin(); i != plot().lines().end(); ++i) {
    MetrixLine& line = static_cast<MetrixLine&>(**i);
    auto title = QString::fromStdWString(line.data_source().title());
    auto width = QFontMetrics(font()).width(title);
    title_width_ = std::max(title_width_, width);
  }

  adjustSize();
  show();
#endif
}

#if defined(UI_QT)
void MetrixGraph::Legend::paintEvent(QPaintEvent* e) {
  QPainter painter(this);

  MetrixGraph& graph = pane().graph();

  //	dc.Rectangle(rect.left, rect.top, rect.right + 1, rect.bottom + 1);

  int top = MARGY;
  for (auto i = plot().lines().begin(); i != plot().lines().end(); i++) {
    MetrixLine& line = static_cast<MetrixLine&>(**i);
    auto& data_source = static_cast<MetrixDataSource&>(line.data_source());

    int left = MARGX;
    for (int i = 0; i < GetColumnCount(); ++i) {
      auto text = QString::fromStdWString(GetText(data_source, i));
      int width = GetColumnWidth(i);
      QRect rect{left, top, width, ROW};
      painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, text);
      left += width + INDENTX;
    }

    top += ROW;
  }
}
#elif defined(UI_VIEWS)
void MetrixGraph::Legend::OnPaint(gfx::Canvas* canvas) {
  MetrixGraph& graph = pane().graph();

  //	dc.Rectangle(rect.left, rect.top, rect.right + 1, rect.bottom + 1);

  int top = MARGY;
  for (views::GraphPlot::Lines::const_iterator i = plot().lines().begin();
       i != plot().lines().end(); i++) {
    MetrixLine& line = static_cast<MetrixLine&>(**i);

    gfx::Rect box(MARGX, top + 3, ROW - 6, ROW - 6);

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
    gfx::Rect rc(p, top, std::max(0, title_width_ - p), ROW);
    canvas->DrawString(title, graph.font_, SK_ColorBLACK, rc,
                       DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Draw value.
    std::wstring text =
        L"= " + line.data_source().timed_data().GetValueString(value.value,
                                                               value.qualifier);
    rc = gfx::Rect(title_width_, top, 80, ROW);
    canvas->DrawString(text, graph.font_, SK_ColorBLACK, rc,
                       DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Draw time.
    std::string time_text = FormatTime(value.source_timestamp);
    p = title_width_ + 80;
    rc = gfx::Rect(p, top, std::max(0, width() - MARGX - p), ROW);
    canvas->DrawString(time_text, graph.font_, SK_ColorBLACK, rc,
                       DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    top += ROW;
  }
}
#endif

#if defined(UI_QT)
QSize MetrixGraph::Legend::sizeHint() const {
  int total_width = MARGX * 2;
  for (int i = 0; i < GetColumnCount(); ++i)
    total_width += GetColumnWidth(i);
  total_width += INDENTX * (GetColumnCount() - 1);

  int total_height = MARGY * 2 + plot().lines().size() * ROW;

  return QSize{total_width, total_height};
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
  width += MARGX * 2 + ROW - 6 + 5;
  int height = plot().lines().empty() ? ROW : ROW * plot().lines().size();
  height += 2 * MARGY;

  const_cast<Legend*>(this)->title_width_ = width;
  return gfx::Size(width + 80 + 150, height);
}
#endif

// MetrixGraph::MetrixLine

MetrixGraph::MetrixLine::MetrixLine() : data_source_(new MetrixDataSource) {
  SetDataSource(data_source_.get());
  set_auto_range(false);
}

MetrixGraph::MetrixLine::~MetrixLine() {
  SetDataSource(nullptr);
}

void MetrixGraph::MetrixLine::OnDataSourceHistoryChanged() {
  views::GraphLine::OnDataSourceHistoryChanged();

  const auto& timed_data = data_source_->timed_data();

  assert(timed_data.historical());

  const auto* values = timed_data.values();
  if (!values || values->empty())
    return;

  // Scroll to last time.
  base::Time last_time = timed_data.current().source_timestamp;
  if (last_time.is_null())
    return;

  double time = last_time.ToDoubleT();
  if (graph().horizontal_axis().panning_range_max() < time)
    graph().horizontal_axis().SetPanningRangeMax(time);
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

MetrixGraph::MetrixGraph(MetrixGraphContext&& context)
    : MetrixGraphContext{std::move(context)} {
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
  if (!data_source().connected())
    return;

  auto& graph_range = graph().horizontal_axis().range();

  auto from = base::Time::FromDoubleT(graph_range.low());
  auto to = graph().m_time_fit ? kTimedDataCurrentOnly
                               : base::Time::FromDoubleT(graph_range.high());

  data_source().SetRange({from, to});
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

MetrixGraph::MetrixLine& MetrixGraph::NewLine(std::string_view path,
                                              MetrixPane& pane) {
  MetrixLine* line = new MetrixLine();
  pane.plot().AddLine(*line);

  TimedDataSpec spec;
  spec.Connect(timed_data_service_, path);
  line->data_source().SetTimedData(std::move(spec));

  return *line;
}

MetrixGraph::MetrixPane& MetrixGraph::NewPane() {
  MetrixPane& pane = *new MetrixPane();

  // TODO: fix this
  /*if (selected_pane_) {
  pane->legend.show = selected_pane_->legend.show;
  pane->cur_box.show = selected_pane_->cur_box.show;
  }*/

  AddPane(pane);

  SelectPane(&pane);

  return pane;
}

void MetrixGraph::Fit() {
  auto range = horizontal_axis().range();

  auto time_max_limit = horizontal_axis().panning_range_max();
  if (m_time_fit && time_max_limit != std::numeric_limits<double>::max()) {
    range = views::GraphRange{time_max_limit - range.delta(), time_max_limit,
                              range.kind()};
  }

  AdjustTimeRange(range);
  horizontal_axis().SetRange(range);
}
