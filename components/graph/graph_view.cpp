#include "components/graph/graph_view.h"

#include "base/color.h"
#include "base/strings/string_util.h"
#include "base/time_utils.h"
#include "base/utils.h"
#include "common/formula_util.h"
#include "common/node_service.h"
#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "components/graph/graph_setup_dialog.h"
#include "components/time_range/time_range_dialog.h"
#include "contents_observer.h"
#include "controller_factory.h"
#include "selection_model.h"
#include "services/profile.h"
#include "time_range.h"
#include "ui/base/models/simple_menu_model.h"

#if defined(UI_QT)
#include "base/qt/color_qt.h"
#include "graph_qt/graph_axis.h"
#include "graph_qt/graph_pane.h"
#include "graph_qt/graph_plot.h"
#elif defined(UI_VIEWS)
#include "skia/ext/skia_utils_win.h"
#include "ui/events/event.h"
#include "ui/gfx/canvas.h"
#include "ui/views/background.h"
#include "ui/views/controls/graph/graph_axis.h"
#include "ui/views/controls/graph/graph_pane.h"
#include "ui/views/controls/graph/graph_plot.h"
#endif

static const size_t kMaxPanes = 10;

std::string FormatTimeDelta(base::TimeDelta delta) {
  int64 s = delta.InSeconds();
  int64 m = s / 60;
  s = s % 60;
  int64 h = m / 60;
  m = m % 60;
  return base::StringPrintf("%d:%02d:%02d", static_cast<int>(h),
                            static_cast<int>(m), static_cast<int>(s));
}

bool ParseTimeDelta(base::StringPiece str, base::TimeDelta& delta) {
  int h, m, s;
  if (sscanf(str.as_string().c_str(), "%d:%d:%d", &h, &m, &s) != 3)
    return false;

  if (h < 0 || m < 0 || s < 0)
    return false;

  delta = base::TimeDelta::FromHours(h) + base::TimeDelta::FromMinutes(m) +
          base::TimeDelta::FromSeconds(s);
  return true;
}

base::string16 FormatTime(base::Time time) {
  base::Time::Exploded e = {0};
  time.UTCExplode(&e);
  return base::StringPrintf(L"%02d-%02d-%04d %02d:%02d:%02d.%03d",
                            e.day_of_month, e.month, e.year, e.hour, e.minute,
                            e.second, e.millisecond);
}

bool ParseTime(base::StringPiece str, base::Time& time) {
  int d, m, y, h, n, s, ms;
  if (sscanf_s(str.as_string().c_str(), "%02d-%02d-%04d %02d:%02d:%02d.%03d",
               &d, &m, &y, &h, &n, &s, &ms) != 7)
    return false;

  base::Time::Exploded e = {0};
  e.year = y;
  e.month = m;
  e.day_of_month = d;
  e.hour = h;
  e.minute = n;
  e.second = s;
  e.millisecond = ms;

  base::Time t;
  if (!base::Time::FromUTCExploded(e, &t))
    return false;

  time = t;
  return true;
}

// GraphView

const WindowInfo kWindowInfo = {
    ID_GRAPH_VIEW, "Graph", L"График", WIN_INS, 0, 0, IDR_GRAPH_POPUP};

REGISTER_CONTROLLER(GraphView, kWindowInfo);

GraphView::GraphView(const ControllerContext& context)
    : ::Controller{context} {}

UiView* GraphView::Init(const WindowDefinition& definition) {
  BOOL time_set = FALSE;
  BOOL val_set = FALSE;

  graph_ =
      std::make_unique<MetrixGraph>(MetrixGraphContext{timed_data_service_});

#if defined(UI_VIEWS)
  graph_->set_background(
      new views::ColorBackground(profile_.graph_view.default_color));
#endif

  typedef std::map<int, views::GraphPane*> PaneMap;
  PaneMap pane_map;

  for (auto& item : definition.items) {
    if (item.name_is("GraphPane")) {
      views::GraphPane* pane = &graph_->NewPane();

      pane->size_percent_ = item.GetInt("size", 100);

      int ix = item.GetInt("ix", -1);
      if (ix != -1)
        pane_map.insert(PaneMap::value_type(ix, pane));
      if (item.GetInt("act", 0))
        graph_->SelectPane(pane);

    } else if (item.name_is("Item")) {
      if (graph_->panes().size() >= kMaxPanes)
        continue;
      auto path = item.GetString("path");
      auto stype = item.GetString("type", "GraphLine");
      auto color_string = item.GetString("clr");
      bool dots = item.GetInt("dots", 1) != 0;
      bool stepped = item.GetInt("stepped", 1) != 0;
      // pane
      int pane_ix = item.GetInt("pane", -1);
      PaneMap::iterator i = pane_map.find(pane_ix);
      MetrixGraph::MetrixPane* pane = NULL;
      if (i != pane_map.end())
        pane = static_cast<MetrixGraph::MetrixPane*>(i->second);
      else
        pane = &static_cast<MetrixGraph::MetrixPane&>(graph_->NewPane());
      // make color
      auto color = color_string.empty() ? NewColor()
                                        : palette::StringToColor(color_string);
      // add line
      MetrixGraph::MetrixLine& line =
          graph_->NewLine(path, *static_cast<MetrixGraph::MetrixPane*>(pane));
      line.color = color;
      line.set_dots_shown(dots);
      line.set_stepped(stepped);

    } else if (item.name_is("TimeScale")) {
      auto srange = item.GetString("span");
      auto stime = item.GetString("time");
      base::Time from, to;
      graph_->m_time_fit = base::EqualsCaseInsensitiveASCII(stime, "Now");
      if (graph_->m_time_fit || !ParseTime(stime, to)) {
        graph_->m_time_fit = true;
        to = base::Time::Now();
      }
      base::TimeDelta span = base::TimeDelta::FromHours(1);
      ParseTimeDelta(srange, span);
      from = to - span;
      graph_->horizontal_axis().SetRange(views::GraphRange(
          from.ToDoubleT(), to.ToDoubleT(), views::GraphRange::TIME));
      time_set = TRUE;
    }
  }

  if (!time_set) {
    base::Time now = base::Time::Now();
    graph_->horizontal_axis().SetRange(
        views::GraphRange((now - profile_.graph_view.default_span).ToDoubleT(),
                          now.ToDoubleT(), views::GraphRange::TIME));
  }

  graph_->right_range_limit_ = graph_->horizontal_axis().range().high();
  graph_->Fit();

  graph_->UpdateData();

  for (MetrixGraph::Panes::const_iterator i = graph_->panes().begin();
       i != graph_->panes().end(); i++) {
    MetrixGraph::MetrixPane& pane = *static_cast<MetrixGraph::MetrixPane*>(*i);
    pane.ShowLegend(true);
  }

  // Select first item.
  if (!graph_->panes().empty())
    graph_->SelectPane(graph_->panes().front());

  OnGraphSelectPane();

  // Don't set controller until graph is initialized.
  graph_->set_controller(this);

  graph_->SetContextMenuHandler([this](const UiPoint& point) {
    controller_delegate_.ShowPopupMenu(0, point, true);
  });

  return graph_.get();
}

bool GraphView::FindColor(SkColor color) const {
  for (MetrixGraph::Panes::const_iterator i = graph_->panes().begin();
       i != graph_->panes().end(); ++i) {
    views::GraphPane& pane = **i;
    const views::GraphPlot::Lines& lines = pane.plot().lines();
    for (views::GraphPlot::Lines::const_iterator i = lines.begin();
         i != lines.end(); ++i)
      if ((*i)->color == color)
        return true;
  }
  return false;
}

SkColor GraphView::NewColor() const {
  for (unsigned i = 0; i < palette::GetColorCount(); i++) {
    SkColor color = palette::GetColor(i);
    if (color == SK_ColorWHITE)
      continue;
    if (!FindColor(color))
      return color;
  }
  return palette::GetColor(rand() % palette::GetColorCount());
}

void GraphView::Save(WindowDefinition& definition) {
  base::Time time =
      base::Time::FromDoubleT(graph_->horizontal_axis().range().high());
  base::TimeDelta span =
      TimeDeltaFromSecondsF(graph_->horizontal_axis().range().delta());

  // time scale
  WindowItem& item = definition.AddItem("TimeScale");
  item.SetString(
      "time", graph_->m_time_fit ? base::string16(L"Now") : FormatTime(time));
  item.SetString("span", FormatTimeDelta(span));

  // value scale
  // WindowItem& item = def.AddItem(_T("ValueScale"));
  int pane_ix = 1;
  // panes
  for (MetrixGraph::Panes::const_iterator i = graph_->panes().begin();
       i != graph_->panes().end(); ++i) {
    views::GraphPane& pane = **i;

    WindowItem& item = definition.AddItem("GraphPane");
    item.SetInt("ix", pane_ix);
    item.SetInt("size", pane.size_percent_);

    const views::GraphPlot::Lines& lines = pane.plot().lines();
    for (views::GraphPlot::Lines::const_iterator i = lines.begin();
         i != lines.end(); ++i) {
      const MetrixGraph::MetrixLine& line =
          static_cast<const MetrixGraph::MetrixLine&>(**i);

      WindowItem& item = definition.AddItem("Item");
      item.SetInt("pane", pane_ix);
      item.SetString("path", line.data_source().GetPath());
#if defined(UI_VIEWS)
      item.SetString("clr", palette::ColorToString(line.color));
#endif
      item.SetInt("dots", line.dots_shown() ? 1 : 0);
      item.SetInt("stepped", line.stepped() ? 1 : 0);
    }

    pane_ix++;
  }
}

void GraphView::DeleteSelectedPane() {
  MetrixGraph::MetrixPane* pane = graph_->selected_pane();
  assert(pane);

  if (!pane)
    return;

  // Deselect current pane.
  views::GraphPane* next = graph_->GetNextPane(pane);
  if (!next)
    next = graph_->GetPrevPane(pane);
  graph_->SelectPane(next);

  // Delete pane.
  ClearPane(*pane);
  graph_->DeletePane(*pane);

  controller_delegate_.SetModified(true);
}

void GraphView::ClearPane(MetrixGraph::MetrixPane& pane) {
  const views::GraphPlot::Lines& lines = pane.plot().lines();
  while (!lines.empty()) {
    MetrixGraph::MetrixLine& line =
        static_cast<MetrixGraph::MetrixLine&>(*lines.front());
    // TODO: Check if there are still another lines for this item.
    NotifyContainedItemChanged(line.data_source().trid(), false);
    pane.plot().DeleteLine(line);
  }
}

void GraphView::AddContainedItem(const scada::NodeId& node_id, unsigned flags) {
  auto color = NewColor();

  MetrixGraph::MetrixPane* pane = NULL;

  // find first empty pane
  for (MetrixGraph::Panes::const_iterator i = graph_->panes().begin();
       i != graph_->panes().end(); ++i) {
    if ((*i)->plot().lines().empty()) {
      pane = static_cast<MetrixGraph::MetrixPane*>(*i);
      break;
    }
  }

  if (!pane && !(flags & APPEND))
    pane = graph_->selected_pane();

  if (!pane)
    pane = &graph_->NewPane();

  const views::GraphPlot::Lines& lines = pane->plot().lines();
  if (!lines.empty()) {
    // empty selected pane
#if defined(UI_QT)
    color = ColorFromQt(lines.front()->color);
#elif defined(UI_VIEWS)
    color = lines.front()->color;
#endif
    ClearPane(*pane);
  }

  std::string path = MakeNodeIdFormula(node_id);

  MetrixGraph::MetrixLine& line =
      graph_->NewLine(path, *static_cast<MetrixGraph::MetrixPane*>(pane));
  line.color = color;
  line.UpdateTimeRange();
  graph_->Fit();

  NotifyContainedItemChanged(line.data_source().trid(), true);

  pane->ShowLegend(true);

#if defined(UI_VIEWS)
  pane->SchedulePaint();
  graph_->Layout();
#endif

  controller_delegate_.SetTitle(MakeTitle());
  controller_delegate_.SetModified(true);
}

base::string16 GraphView::MakeTitle() const {
  MetrixGraph::MetrixLine* line =
      !graph_->panes().empty()
          ? static_cast<MetrixGraph::MetrixLine*>(
                graph_->panes().front()->plot().primary_line())
          : NULL;
  return line ? line->data_source().title() : L"Нет объекта";
}

void GraphView::ShowSetupDialog() {
#if defined(UI_VIEWS)
  MetrixGraph::MetrixLine* line =
      graph_->selected_pane() ? graph_->selected_pane()->primary_line() : NULL;

  const views::ColorBackground* background =
      static_cast<const views::ColorBackground*>(graph_->background());
  SkColor color = background ? background->color() : SK_ColorWHITE;

  GraphSetupDialog dlg;
  dlg.color = color;
  dlg.line_weight_ = line ? line->line_weight_ : 0;

  if (dlg.DoModal() != IDOK)
    return;

  graph_->set_background(new views::ColorBackground(dlg.color));
  profile_.graph_view.default_color = dlg.color;

  if (line)
    line->line_weight_ = dlg.line_weight_;

  graph_->SchedulePaint();
#endif
}

bool GraphView::CanClose() const {
  return true;
}

bool GraphView::IsWorking() const {
  for (MetrixGraph::Panes::const_iterator i = graph_->panes().begin();
       i != graph_->panes().end(); ++i) {
    views::GraphPane& pane = **i;
    const views::GraphPlot::Lines& lines = pane.plot().lines();
    for (views::GraphPlot::Lines::const_iterator i = lines.begin();
         i != lines.end(); ++i) {
      MetrixGraph::MetrixLine& line =
          static_cast<MetrixGraph::MetrixLine&>(**i);
      if (!line.data_source().is_ready())
        return true;
    }
  }
  return false;
}

void GraphView::OnGraphSelectPane() {
  MetrixGraph::MetrixLine* line =
      graph_->selected_pane()
          ? static_cast<MetrixGraph::MetrixPane*>(graph_->selected_pane())
                ->primary_line()
          : NULL;
  if (line)
    selection().SelectTimedData(line->data_source().timed_data());
  else
    selection().Clear();
}

TimeRange GraphView::GetTimeRange() const {
  auto start = base::Time::FromDoubleT(graph_->horizontal_axis().range().low());
  base::Time end;
  if (!graph_->m_time_fit)
    end = base::Time::FromDoubleT(graph_->horizontal_axis().range().high());
  return TimeRange{start, end};
}

NodeIdSet GraphView::GetContainedItems() const {
  NodeIdSet items;
  for (MetrixGraph::Panes::const_iterator i = graph_->panes().begin();
       i != graph_->panes().end(); ++i) {
    views::GraphPane& pane = **i;
    const views::GraphPlot::Lines& lines = pane.plot().lines();
    for (views::GraphPlot::Lines::const_iterator i = lines.begin();
         i != lines.end(); ++i) {
      MetrixGraph::MetrixLine& line =
          static_cast<MetrixGraph::MetrixLine&>(**i);
      scada::NodeId trid = line.data_source().trid();
      if (trid != scada::NodeId())
        items.insert(trid);
    }
  }
  return items;
}

void GraphView::RemoveContainedItem(const scada::NodeId& node_id) {
  for (MetrixGraph::Panes::const_iterator i = graph_->panes().begin();
       i != graph_->panes().end();) {
    views::GraphPane& pane = **i++;
    const views::GraphPlot::Lines& lines = pane.plot().lines();
    for (views::GraphPlot::Lines::const_iterator i = lines.begin();
         i != lines.end();) {
      MetrixGraph::MetrixLine& line =
          static_cast<MetrixGraph::MetrixLine&>(**i++);

      if (line.data_source().trid() == node_id)
        pane.plot().DeleteLine(line);
    }

    // Delete pane if empty. But don't delete last one.
    if (pane.plot().lines().empty()) {
      if (&pane == graph_->selected_pane())
        graph_->SelectPane(NULL);
      graph_->DeletePane(pane);
    }
  }

  // All lines corresponding to |item| were removed.
  NotifyContainedItemChanged(node_id, false);
}

CommandHandler* GraphView::GetCommandHandler(unsigned command_id) {
  switch (command_id) {
    case ID_VIEW_LEGEND:
      return graph_->selected_pane() ? this : NULL;

    case ID_GRAPH_DOTS:
    case ID_GRAPH_STEPS:
      return graph_->primary_line() ? this : NULL;

    case ID_NOW:
    case ID_GRAPH_ADD_PANE:
    case ID_GRAPH_DELETE_PANE:
    case ID_GRAPH_ZOOM:
      return this;
  }

  if (command_id >= ID_COLOR_0 &&
      command_id < ID_COLOR_0 + palette::GetColorCount())
    return this;

  return __super::GetCommandHandler(command_id);
}

bool GraphView::IsCommandChecked(unsigned command_id) const {
  switch (command_id) {
    case ID_GRAPH_ZOOM:
      return graph_->selected_pane() &&
             graph_->selected_pane()->plot().zooming();

    case ID_VIEW_LEGEND:
      return graph_->selected_pane() && graph_->selected_pane()->show_legend();

    case ID_GRAPH_DOTS:
      return graph_->primary_line() && graph_->primary_line()->dots_shown();
    case ID_GRAPH_STEPS:
      return graph_->primary_line() && graph_->primary_line()->stepped();

    case ID_NOW:
      return graph_->m_time_fit;

    default:
      return __super::IsCommandChecked(command_id);
  }
}

void GraphView::ExecuteCommand(unsigned command_id) {
  switch (command_id) {
    case ID_NOW:
      if (!graph_->m_time_fit) {
        graph_->m_time_fit = true;
        graph_->Fit();
#if defined(UI_QT)
        graph_->update();
#elif defined(UI_VIEWS)
        graph_->SchedulePaint();
#endif
        controller_delegate_.SetModified(true);
      }
      break;

    case ID_GRAPH_ADD_PANE: {
      controller_delegate_.SetModified(true);
      views::GraphPane& pane = graph_->NewPane();
      // TODO: Recover prompt.
      //      PromptBegin(views::View::GetWindowHandle(),
      //          pane.rect_.left + 10, pane.rect_.top + 10);
      break;
    }

    case ID_GRAPH_DELETE_PANE:
      DeleteSelectedPane();
      break;

    case ID_VIEW_LEGEND:
      if (MetrixGraph::MetrixPane* pane = graph_->selected_pane()) {
        pane->ShowLegend(!pane->show_legend());
#if defined(UI_VIEWS)
        graph_->SchedulePaint();
#endif
        controller_delegate_.SetModified(true);
      }
      break;

    case ID_GRAPH_DOTS:
    case ID_GRAPH_STEPS:
      if (MetrixGraph::MetrixLine* line = graph_->primary_line()) {
        switch (command_id) {
          case ID_GRAPH_DOTS:
            line->set_dots_shown(!line->dots_shown());
            break;
          case ID_GRAPH_STEPS:
            line->set_stepped(!line->stepped());
            break;
        }
#if defined(UI_VIEWS)
        graph_->SchedulePaint();
#endif
        controller_delegate_.SetModified(true);
      }
      break;

    case ID_GRAPH_ZOOM:
      if (graph_->selected_pane()) {
        graph_->selected_pane()->plot().set_zooming(
            !graph_->selected_pane()->plot().zooming());
        if (graph_->selected_pane()->plot().zooming()) {
          graph_->m_time_fit = false;
          prezoom_horizontal_range_ = graph_->horizontal_axis().range();
        } else
          UndoZoom();
      }
      break;

    default:
      if (command_id >= ID_COLOR_0 &&
          command_id < ID_COLOR_0 + palette::GetColorCount()) {
        SkColor color = palette::GetColor(command_id - ID_COLOR_0);
        if (MetrixGraph::MetrixPane* pane = graph_->selected_pane()) {
          views::GraphLine* line = pane->plot().lines().front();
          line->color = color;
#if defined(UI_VIEWS)
          pane->SchedulePaint();
#endif
        }
      } else {
        __super::ExecuteCommand(command_id);
      }
      break;
  }
}

void GraphView::SetTimeRange(const TimeRange& range) {
  auto [start_time, end_time] = GetTimeRangeBounds(range);

  double low = start_time.ToDoubleT();

  graph_->m_time_fit = end_time.is_null();
  double high =
      graph_->m_time_fit ? graph_->right_range_limit_ : end_time.ToDoubleT();

  graph_->horizontal_axis().SetRange(
      views::GraphRange(low, high, views::GraphRange::TIME));

  controller_delegate_.SetModified(true);
}

void GraphView::OnLineItemChanged(views::GraphLine& line) {
  if (&line == graph_->primary_line())
    controller_delegate_.SetTitle(MakeTitle());

  MetrixGraph::MetrixLine& metrix_line =
      static_cast<MetrixGraph::MetrixLine&>(line);
  auto node_id = metrix_line.data_source().timed_data().GetNode().node_id();
  NotifyContainedItemChanged(node_id, true);
}

void GraphView::UndoZoom() {
  graph_->horizontal_axis().SetRange(prezoom_horizontal_range_);
  graph_->m_time_fit = true;
  graph_->Fit();

  for (MetrixGraph::Panes::const_iterator i = graph_->panes().begin();
       i != graph_->panes().end(); ++i) {
    (*i)->vertical_axis().UpdateRange();
  }
}

void GraphView::OnGraphModified() {
  controller_delegate_.SetModified(true);

  // update defaults
  base::TimeDelta span =
      base::Time::FromDoubleT(graph_->horizontal_axis().range().high()) -
      base::Time::FromDoubleT(graph_->horizontal_axis().range().low());
  if (span.InSeconds() >= 1)
    profile_.graph_view.default_span = span;
}
