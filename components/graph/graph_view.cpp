#include "components/graph/graph_view.h"

#include "base/string_piece_util.h"
#include "base/strings/string_util.h"
#include "base/time_utils.h"
#include "common/formula_util.h"
#include "common_resources.h"
#include "components/time_range/time_range_dialog.h"
#include "controller_delegate.h"
#include "controls/color.h"
#include "model/scada_node_ids.h"
#include "node_service/node_service.h"
#include "selection_model.h"
#include "services/profile.h"
#include "time_range.h"
#include "window_definition_util.h"

#if defined(UI_VIEWS)
#include "components/graph/graph_setup_dialog.h"
#elif defined(UI_QT)
#include <QColorDialog>
#endif

static const size_t kMaxPanes = 10;

// GraphView

GraphView::GraphView(const ControllerContext& context)
    : ControllerContext{context} {}

UiView* GraphView::Init(const WindowDefinition& definition) {
  BOOL time_set = FALSE;

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
      auto color =
          color_string.empty() ? NewColor() : aui::StringToColor(color_string);
      // add line
      MetrixGraph::MetrixLine& line =
          graph_->NewLine(path, *static_cast<MetrixGraph::MetrixPane*>(pane));
      line.SetColor(color.native_color());
      line.set_dots_shown(dots);
      line.set_stepped(stepped);

    } else if (item.name_is("TimeScale")) {
      auto srange = item.GetString("span");
      auto stime = item.GetString("time");
      base::Time from, to;
      graph_->m_time_fit =
          base::EqualsCaseInsensitiveASCII(ToStringPiece(stime), "Now");
      if (graph_->m_time_fit || !Deserialize(stime, to)) {
        graph_->m_time_fit = true;
        to = base::Time::Now();
      }
      base::TimeDelta span = base::TimeDelta::FromHours(1);
      Deserialize(srange, span);
      from = to - span;
      graph_->horizontal_axis().SetRange(views::GraphRange(
          from.ToDoubleT(), to.ToDoubleT(), views::GraphRange::TIME));
      time_set = TRUE;
    }
  }

  if (!time_set) {
    if (auto time_range = RestoreTimeRange(definition)) {
      graph_->m_time_fit = time_range->type != TimeRange::Type::Custom;
      auto [start, end] = GetTimeRangeBounds(*time_range);
      graph_->horizontal_axis().SetRange(views::GraphRange(
          start.ToDoubleT(), end.ToDoubleT(), views::GraphRange::TIME));
      time_set = TRUE;
    } else {
      base::Time now = base::Time::Now();
      graph_->horizontal_axis().SetRange(views::GraphRange(
          (now - profile_.graph_view.default_span).ToDoubleT(), now.ToDoubleT(),
          views::GraphRange::TIME));
    }
  }

  graph_->horizontal_axis().SetPanningRangeMax(
      graph_->horizontal_axis().range().high());
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

  command_handler_.AddCommand(
      Command{ID_NOW}
          .set_execute_handler([this] { ScrollToNow(); })
          .set_checked_handler([this] { return graph_->m_time_fit; }));

  command_handler_.AddCommand(
      Command{ID_GRAPH_ADD_PANE}.set_execute_handler([this] {
        controller_delegate_.SetModified(true);
        graph_->NewPane();
        // TODO: Recover prompt.
        //      PromptBegin(views::View::GetWindowHandle(),
        //          pane.rect_.left + 10, pane.rect_.top + 10);
      }));

  const Command::EnabledHandler selected_pane_enabled_handler = [this] {
    return !!graph_->selected_pane();
  };

  command_handler_.AddCommand(
      Command{ID_GRAPH_DELETE_PANE}
          .set_execute_handler([this] { DeleteSelectedPane(); })
          .set_enabled_handler(selected_pane_enabled_handler));

  command_handler_.AddCommand(
      Command{ID_VIEW_LEGEND}
          .set_execute_handler([this] { ToggleLegend(); })
          .set_enabled_handler(selected_pane_enabled_handler)
          .set_checked_handler([this] {
            return graph_->selected_pane() &&
                   graph_->selected_pane()->show_legend();
          }));

  command_handler_.AddCommand(
      Command{ID_GRAPH_ZOOM}
          .set_execute_handler([this] { ToggleZoom(); })
          .set_enabled_handler(selected_pane_enabled_handler)
          .set_checked_handler([this] {
            return graph_->selected_pane() &&
                   graph_->selected_pane()->plot().zooming();
          }));

  const Command::EnabledHandler line_enabled_handler = [this] {
    return !!graph_->selected_pane();
  };

  command_handler_.AddCommand(
      Command{ID_GRAPH_COLOR}
          .set_execute_handler([this] { ChooseLineColor(); })
          .set_enabled_handler(line_enabled_handler));

  command_handler_.AddCommand(
      Command{ID_GRAPH_DOTS}
          .set_execute_handler([this] { ToggleLineProperty(ID_GRAPH_DOTS); })
          .set_enabled_handler(line_enabled_handler)
          .set_checked_handler([this] {
            return graph_->primary_line() &&
                   graph_->primary_line()->dots_shown();
          }));

  command_handler_.AddCommand(
      Command{ID_GRAPH_STEPS}
          .set_execute_handler([this] { ToggleLineProperty(ID_GRAPH_STEPS); })
          .set_enabled_handler(line_enabled_handler)
          .set_checked_handler([this] {
            return graph_->primary_line() && graph_->primary_line()->stepped();
          }));

  return graph_.get();
}

bool GraphView::FindColor(aui::Color color) const {
  for (auto* pane : graph_->panes()) {
    for (auto* line : pane->plot().lines())
      if (aui::Color::FromNativeColor(line->color()) == color)
        return true;
  }
  return false;
}

aui::Color GraphView::NewColor() const {
  for (unsigned i = 0; i < aui::GetColorCount(); i++) {
    const auto color = aui::GetColor(i);
    if (color == aui::ColorCode::White || color == aui::ColorCode::Transparent)
      continue;
    if (!FindColor(color))
      return color;
  }
  return aui::GetColor(rand() % aui::GetColorCount());
}

void GraphView::Save(WindowDefinition& definition) {
  base::Time time =
      base::Time::FromDoubleT(graph_->horizontal_axis().range().high());
  base::TimeDelta span =
      TimeDeltaFromSecondsF(graph_->horizontal_axis().range().delta());

  // time scale
  {
    WindowItem& item = definition.AddItem("TimeScale");
    item.SetString("time", graph_->m_time_fit ? std::string("Now")
                                              : SerializeToString(time));
    item.SetString("span", SerializeToString(span));
  }

  // value scale
  // WindowItem& item = def.AddItem(_T("ValueScale"));
  int pane_ix = 1;
  // panes
  for (auto* pane : graph_->panes()) {
    {
      WindowItem& item = definition.AddItem("GraphPane");
      item.SetInt("ix", pane_ix);
      item.SetInt("size", pane->size_percent_);
    }

    for (auto* graph_line : pane->plot().lines()) {
      const MetrixGraph::MetrixLine& line =
          static_cast<const MetrixGraph::MetrixLine&>(*graph_line);

      WindowItem& item = definition.AddItem("Item");
      item.SetInt("pane", pane_ix);
      item.SetString("path", line.data_source().GetPath());
      item.SetString(
          "clr", aui::ColorToString(aui::Color::FromNativeColor(line.color())));
      item.SetInt("dots", line.dots_shown() ? 1 : 0);
      item.SetInt("stepped", line.stepped() ? 1 : 0);
    }

    pane_ix++;
  }

  SaveTimeRange(definition, GetTimeRange());
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

  const auto& lines = pane->plot().lines();
  if (!lines.empty()) {
    // empty selected pane
    color = aui::Color::FromNativeColor(lines.front()->color());
    ClearPane(*pane);
  }

  std::string path = MakeNodeIdFormula(node_id);

  auto& line =
      graph_->NewLine(path, *static_cast<MetrixGraph::MetrixPane*>(pane));
  line.SetColor(color.native_color());
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

std::wstring GraphView::MakeTitle() const {
  MetrixGraph::MetrixLine* line =
      !graph_->panes().empty()
          ? static_cast<MetrixGraph::MetrixLine*>(
                graph_->panes().front()->plot().primary_line())
          : NULL;
  return line ? line->data_source().title() : L"Нет объекта";
}

bool GraphView::CanClose() const {
  return true;
}

bool GraphView::IsWorking() const {
  for (auto* pane : graph_->panes()) {
    for (auto* graph_line : pane->plot().lines()) {
      auto& line = static_cast<MetrixGraph::MetrixLine&>(*graph_line);
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
    selection_.SelectTimedData(line->data_source().timed_data());
  else
    selection_.Clear();
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
  for (auto* pane : graph_->panes()) {
    for (auto* graph_line : pane->plot().lines()) {
      auto& line = static_cast<MetrixGraph::MetrixLine&>(*graph_line);
      const auto& node_id = line.data_source().trid();
      if (!node_id.is_null())
        items.insert(node_id);
    }
  }
  return items;
}

void GraphView::RemoveContainedItem(const scada::NodeId& node_id) {
  for (auto i = graph_->panes().begin(); i != graph_->panes().end();) {
    auto& pane = **i++;
    const auto& lines = pane.plot().lines();
    for (auto j = lines.begin(); j != lines.end();) {
      auto& line = static_cast<MetrixGraph::MetrixLine&>(**j++);

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
  return command_handler_.GetCommandHandler(command_id);
}

void GraphView::ScrollToNow() {
  if (graph_->m_time_fit)
    return;

  graph_->m_time_fit = true;
  graph_->Fit();

#if defined(UI_QT)
  graph_->update();
#elif defined(UI_VIEWS)
  graph_->SchedulePaint();
#endif

  controller_delegate_.SetModified(true);
}

void GraphView::ToggleLegend() {
  MetrixGraph::MetrixPane* pane = graph_->selected_pane();
  if (!pane)
    return;

  pane->ShowLegend(!pane->show_legend());

#if defined(UI_VIEWS)
  graph_->SchedulePaint();
#endif

  controller_delegate_.SetModified(true);
}

void GraphView::ToggleLineProperty(unsigned command_id) {
  MetrixGraph::MetrixLine* line = graph_->primary_line();
  if (!line)
    return;

  switch (command_id) {
    case ID_GRAPH_DOTS:
      line->set_dots_shown(!line->dots_shown());
      break;
    case ID_GRAPH_STEPS:
      line->set_stepped(!line->stepped());
      break;
    default:
      assert(false);
      return;
  }

#if defined(UI_VIEWS)
  graph_->SchedulePaint();
#endif

  controller_delegate_.SetModified(true);
}

void GraphView::ToggleZoom() {
  if (!graph_->selected_pane())
    return;

  graph_->selected_pane()->plot().set_zooming(
      !graph_->selected_pane()->plot().zooming());

  if (graph_->selected_pane()->plot().zooming()) {
    graph_->m_time_fit = false;
    prezoom_horizontal_range_ = graph_->horizontal_axis().range();
  } else {
    UndoZoom();
  }
}

void GraphView::SetTimeRange(const TimeRange& range) {
  graph_->m_time_fit = range.type != TimeRange::Type::Custom;
  auto [start_time, end_time] = GetTimeRangeBounds(range);
  double low = start_time.ToDoubleT();
  double high = graph_->m_time_fit
                    ? graph_->horizontal_axis().panning_range_max()
                    : end_time.ToDoubleT();
  graph_->horizontal_axis().SetRange(
      views::GraphRange(low, high, views::GraphRange::TIME));

  controller_delegate_.SetModified(true);
}

void GraphView::OnGraphPannedHorizontally() {
  graph_->m_time_fit = false;
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

void GraphView::ChooseLineColor() {
  auto* line = graph_->primary_line();
  if (!line)
    return;

#if defined(UI_QT)
  auto new_color = QColorDialog::getColor(line->color(), graph_.get());
  if (new_color.isValid())
    line->SetColor(new_color);
#endif
}
