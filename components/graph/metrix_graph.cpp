#include "components/graph/metrix_graph.h"

#include "base/auto_reset.h"
#include "base/format_time.h"
#include "base/minute_time.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/scoped_handle.h"
#include "components/graph/metrix_data_source.h"

#if defined(UI_QT)
#include "graph_qt/graph_axis.h"
#include "graph_qt/graph_cursor.h"
#endif

#include <algorithm>

#if defined(UI_QT)
#include <QPainter>
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

  return static_cast<int>(ready / total * 100);
}

std::string FormatTime(base::Time time, const char* format_string) {
  if (strcmp(format_string, "ms") == 0) {
    base::Time::Exploded e = {0};
    time.LocalExplode(&e);
    return base::StringPrintf("%d:%02d.%03d", e.minute, e.second,
                              e.millisecond);
  } else {
    char buf[128];
    time_t t = time.ToTimeT();
#if defined(WIN32)
    tm ttmv;
    tm* ttm = localtime_s(&ttmv, &t) == 0 ? &ttmv : nullptr;
#else
    tm* ttm = localtime(&t);
#endif
    if (!ttm)
      return std::string();
    size_t size = strftime(buf, sizeof(buf), format_string, ttm);
    return std::string(buf, buf + size);
  }
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

std::u16string MetrixGraph::Legend::GetText(const MetrixDataSource& data_source,
                                            int column_id) const {
  switch (column_id) {
    case 0: {
      return data_source.title();
    }
    case 1: {
      auto data_value = GetCurrentValue(data_source);
      return u"= " + data_source.timed_data().GetValueString(
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
#if defined(UI_QT)
  title_width_ = 0;
  for (auto i = plot().lines().begin(); i != plot().lines().end(); ++i) {
    MetrixLine& line = static_cast<MetrixLine&>(**i);
    auto title = QString::fromStdU16String(line.data_source().title());
    auto width = QFontMetrics(font()).horizontalAdvance(title);
    title_width_ = std::max(title_width_, width);
  }

  adjustSize();
  show();
#endif
}

#if defined(UI_QT)
void MetrixGraph::Legend::paintEvent(QPaintEvent* e) {
  QPainter painter(this);

  //	dc.Rectangle(rect.left, rect.top, rect.right + 1, rect.bottom + 1);

  int top = MARGY;
  for (auto* graph_line : plot().lines()) {
    MetrixLine& line = static_cast<MetrixLine&>(*graph_line);
    auto& data_source = static_cast<MetrixDataSource&>(line.data_source());

    int left = MARGX;
    for (int i = 0; i < GetColumnCount(); ++i) {
      auto text = QString::fromStdU16String(GetText(data_source, i));
      int width = GetColumnWidth(i);
      QRect rect{left, top, width, ROW};
      painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, text);
      left += width + INDENTX;
    }

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
  for (auto* pane : panes()) {
    for (auto* line : pane->plot().lines())
      static_cast<MetrixLine*>(line)->UpdateTimeRange();
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

  base::AutoReset updating{&updating_, true};
  horizontal_axis().SetRange(range);
}

QString MetrixGraph::GetXAxisLabel(double val) const {
  static const double kSecondStep = 1.0;
  static const double kMinuteStep = 60 * kSecondStep;
  static const double kHourStep = 60 * kMinuteStep;
  static const double kDayStep = 24 * kHourStep;

  // time format
  const char* format_string;
  if (horizontal_axis().tick_step() >= kDayStep)
    format_string = "%#d %b";
  else if (horizontal_axis().tick_step() >= kHourStep)
    format_string = "%#d-%#H:%M";
  else if (horizontal_axis().tick_step() >= kMinuteStep)
    format_string = "%#H:%M";
  else if (horizontal_axis().tick_step() >= kSecondStep)
    format_string = "%#H:%M:%S";
  else
    format_string = "ms";  // special msec format

  return QString::fromStdString(
      FormatTime(base::Time::FromDoubleT(val), format_string));
}

void MetrixGraph::OnGraphPannedHorizontally() {
  if (updating_) {
    return;
  }

  m_time_fit = false;
}
