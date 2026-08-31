#include "graph/metrix_graph.h"
#include "base/time/time_wire_codec.h"

#include "base/format_time.h"
#include "base/minute_time.h"
#include "base/utf_convert.h"
#include "graph/limit_markers.h"
#include "graph/metrix_data_source.h"

#include "graph/series_stats.h"

#if defined(UI_QT)
#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "scada/qualifier.h"
#include "scada/variant.h"
#endif

#include <algorithm>
#include <optional>

#if defined(UI_QT)
#include <QColor>
#include <QEvent>
#include <QPainter>
#include <QPalette>
#include <QString>
#endif

namespace {

#if defined(UI_QT)
// The active theme's tokens that drive the chart chrome. Resolved through
// ActiveThemeTokens() so that under Theme::kSystem the tokens are themselves
// derived from the live palette rather than from a baked light/dark table. See
// the UX design language at docs/client/ux/design-language.md.
const scada::aui::ThemeTokens& ChartTokens() {
  return scada::aui::ActiveThemeTokens();
}

// Formats an already-typed data value through the series' own value formatter
// (engineering units / decimals), matching the legend's live-value column.
QString FormatDataValue(const MetrixDataSource& data_source,
                        const scada::DataValue& value) {
  return QString::fromStdU16String(
      data_source.timed_data().GetValueString(value.value, value.qualifier));
}

// Formats a raw numeric aggregate (min/max/average) through the same formatter
// by wrapping it in a good-quality variant, so stat cells read consistently
// with the live value.
QString FormatNumber(const MetrixDataSource& data_source, double number) {
  return QString::fromStdU16String(data_source.timed_data().GetValueString(
      scada::Variant{number}, scada::Qualifier{}));
}

// Placeholder shown when a cell has no value (no cursor, or no good sample in
// the visible range).
QString EmptyCell() {
  return QString::fromUtf8("\xE2\x80\x94");  // em dash
}

#endif

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
  const GraphCursor* cursor = graph().selected_cursor();
  if (cursor && !cursor->axis_->is_vertical()) {
    scada::Time cursor_time = scada::base::DecodeDoubleT(cursor->position_);
    const scada::DataValue* cursor_value =
        data_source.timed_data().GetValueAt(cursor_time);
    return cursor_value ? *cursor_value : scada::DataValue{};
  } else {
    return data_source.timed_data().current();
  }
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
  PaintThemed(painter);
}

namespace {

// Reshell value-grid geometry (device-independent pixels).
constexpr int kThemedPad = 8;          // outer padding
constexpr int kThemedRow = 16;         // series row height
constexpr int kThemedHeader = 14;      // header row height
constexpr int kThemedSwatchW = 12;     // colour swatch width
constexpr int kThemedSwatchH = 3;      // colour swatch height
constexpr int kThemedSwatchGap = 8;    // gap between swatch and name
constexpr int kThemedNumColW = 68;     // width of a numeric column
constexpr int kThemedCursorColW = 78;  // width of the wider "@ cursor" column

// The five numeric columns to the right of the series name. `header` is an
// English *source*: it is looked up through Translate() at paint time rather
// than baked, so a language switch re-renders the header row. Declaring it
// `const char*` is also what lets rule 7 of check_ui_translations.py resolve
// the strings back to this table -- the painter passes a member, so no rule
// that reads call-site literals can see them (task 418).
struct ThemedColumn {
  const char* header;
  int width;
};
const ThemedColumn kThemedColumns[] = {
    {"Current", kThemedNumColW},     {"Min", kThemedNumColW},
    {"Max", kThemedNumColW},         {"Average", kThemedNumColW},
    {"@ cursor", kThemedCursorColW},
};
constexpr int kThemedColumnCount =
    static_cast<int>(sizeof(kThemedColumns) / sizeof(kThemedColumns[0]));

}  // namespace

void MetrixGraph::Legend::PaintThemed(QPainter& painter) const {
  const scada::aui::ThemeTokens& tokens = ChartTokens();

  painter.setRenderHint(QPainter::Antialiasing, true);

  // Panel background + hairline border so the readout sits legibly over the
  // plotted lines.
  const QRectF panel = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
  painter.setPen(QPen{tokens.border});
  painter.setBrush(tokens.surface_muted);
  painter.drawRoundedRect(panel, 6, 6);

  const int name_col_w = kThemedSwatchW + kThemedSwatchGap + title_width_;
  const int values_left = kThemedPad + name_col_w;

  // The visible range drives the min/max/average aggregates.
  const GraphRange& range = graph().horizontal_axis().range();
  const scada::Time from = scada::base::DecodeDoubleT(range.low());
  const scada::Time to = scada::base::DecodeDoubleT(range.high());

  // Header row: column captions, right-aligned over their numeric columns.
  QFont header_font = painter.font();
  header_font.setPointSizeF(std::max(1.0, header_font.pointSizeF() - 1.0));
  painter.setFont(header_font);
  painter.setPen(QPen{tokens.fg_subtle});
  {
    int left = values_left;
    for (const ThemedColumn& column : kThemedColumns) {
      const QRect cell{left, kThemedPad, column.width, kThemedHeader};
      painter.drawText(cell, Qt::AlignRight | Qt::AlignVCenter,
                       QString::fromStdU16String(Translate(column.header)));
      left += column.width;
    }
  }

  int top = kThemedPad + kThemedHeader;
  for (auto* graph_line : plot().lines()) {
    MetrixLine& line = static_cast<MetrixLine&>(*graph_line);
    const MetrixDataSource& data_source =
        static_cast<const MetrixDataSource&>(line.data_source());

    const SeriesStats stats =
        ComputeSeriesStats(data_source.timed_data().values(), from, to);

    const int mid = top + kThemedRow / 2;

    // Colour swatch matching the series line.
    painter.setPen(Qt::NoPen);
    painter.setBrush(line.color());
    painter.drawRect(kThemedPad, mid - kThemedSwatchH / 2, kThemedSwatchW,
                     kThemedSwatchH);

    // Series name.
    QFont name_font = painter.font();
    name_font.setPointSizeF(header_font.pointSizeF() + 1.0);
    painter.setFont(name_font);
    painter.setPen(QPen{tokens.fg});
    const QRect name_rect{kThemedPad + kThemedSwatchW + kThemedSwatchGap, top,
                          title_width_, kThemedRow};
    painter.drawText(name_rect, Qt::AlignLeft | Qt::AlignVCenter,
                     QString::fromStdU16String(data_source.title()));

    // Value cells.
    const scada::DataValue current = data_source.timed_data().current();
    const QString cells[kThemedColumnCount] = {
        FormatDataValue(data_source, current),
        stats.valid ? FormatNumber(data_source, stats.min) : EmptyCell(),
        stats.valid ? FormatNumber(data_source, stats.max) : EmptyCell(),
        stats.valid ? FormatNumber(data_source, stats.average) : EmptyCell(),
        ValueAtCursorText(data_source),
    };

    int left = values_left;
    for (int i = 0; i < kThemedColumnCount; ++i) {
      // The @-cursor column is accented when populated to echo the plot cursor.
      const bool is_cursor = i == kThemedColumnCount - 1;
      const bool populated = cells[i] != EmptyCell();
      painter.setPen(QPen{is_cursor && populated ? tokens.accent
                          : populated            ? tokens.fg
                                                 : tokens.fg_subtle});
      const QRect cell{left, top, kThemedColumns[i].width, kThemedRow};
      painter.drawText(cell, Qt::AlignRight | Qt::AlignVCenter, cells[i]);
      left += kThemedColumns[i].width;
    }

    top += kThemedRow;
  }
}

QString MetrixGraph::Legend::ValueAtCursorText(
    const MetrixDataSource& data_source) const {
  const GraphCursor* cursor = graph().selected_cursor();
  if (!cursor || cursor->axis_->is_vertical())
    return EmptyCell();
  const scada::Time cursor_time = scada::base::DecodeDoubleT(cursor->position_);
  const scada::DataValue* value =
      data_source.timed_data().GetValueAt(cursor_time);
  if (!value)
    return EmptyCell();
  return FormatDataValue(data_source, *value);
}

QSize MetrixGraph::Legend::ThemedSize() const {
  int values_w = 0;
  for (const ThemedColumn& column : kThemedColumns)
    values_w += column.width;

  const int name_col_w = kThemedSwatchW + kThemedSwatchGap + title_width_;
  const int width = kThemedPad * 2 + name_col_w + values_w;
  const int height = kThemedPad * 2 + kThemedHeader +
                     static_cast<int>(plot().lines().size()) * kThemedRow;
  return QSize{width, height};
}
#endif

#if defined(UI_QT)
QSize MetrixGraph::Legend::sizeHint() const {
  return ThemedSize();
}
#endif

// MetrixGraph::MetrixLine

MetrixGraph::MetrixLine::MetrixLine()
    : data_source_{std::make_unique<MetrixDataSource>()} {
  SetDataSource(data_source_.get());
  set_auto_range(false);
}

MetrixGraph::MetrixLine::~MetrixLine() {
  SetDataSource(nullptr);
}

void MetrixGraph::MetrixLine::OnDataSourceCurrentValueChanged() {
  GraphLine::OnDataSourceCurrentValueChanged();

  if (!graph().selected_cursor())
    graph().UpdateCurBox();
}

void MetrixGraph::MetrixLine::OnDataSourceItemChanged() {
  GraphLine::OnDataSourceItemChanged();

  UpdateLimitStyles();
  pane().UpdateLegend();

  if (graph().controller())
    graph().controller()->OnLineItemChanged(*this);
}

void MetrixGraph::MetrixLine::UpdateLimitStyles() {
  // The loop below casts LimitKind -> GraphLine::LimitBand by value, relying on
  // the two enums listing the same four bands in the same order. Guard that at
  // compile time so a reorder in either (LimitKind here, LimitBand in the
  // external graph_qt) is a build error, not a silent severity mis-map.
  static_assert(static_cast<int>(LimitKind::kLoLo) ==
                static_cast<int>(LimitBand::kLoLo));
  static_assert(static_cast<int>(LimitKind::kLo) ==
                static_cast<int>(LimitBand::kLo));
  static_assert(static_cast<int>(LimitKind::kHi) ==
                static_cast<int>(LimitBand::kHi));
  static_assert(static_cast<int>(LimitKind::kHiHi) ==
                static_cast<int>(LimitBand::kHiHi));

  ClearLimitStyles();

  const MetrixDataSource& source = *data_source_;
  const std::vector<LimitMarker> markers = ComputeLimitMarkers(
      source.limit_lolo(), source.limit_lo(), source.limit_hi(),
      source.limit_hihi(), kGraphUnknownValue);

  for (const LimitMarker& marker : markers) {
    const std::optional<scada::aui::Color> color =
        scada::aui::SeverityColor(SeverityOf(marker.kind));
    // Defensive: every real LimitKind maps to Warning or Critical, so this
    // only fires on a band with no severity at all.
    if (!color)
      continue;

    LimitStyle style;
    style.color = color->qcolor();
    style.label =
        QString::fromStdU16String(Translate(LimitBandNameKey(marker.kind))) +
        QStringLiteral(" ") + source.GetYAxisLabel(marker.value);
    // LimitKind and GraphLine::LimitBand enumerate the same four bands in the
    // same order.
    SetLimitStyle(static_cast<LimitBand>(marker.kind), style);
  }
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
#if defined(UI_QT)
  ApplyChartPalette();
#endif

  QObject::connect(&update_data_timer_, &QTimer::timeout,
                   [this] { UpdateData(); });
  update_data_timer_.start(50);
}

#if defined(UI_QT)

void MetrixGraph::changeEvent(QEvent* event) {
  Graph::changeEvent(event);

  // An OS appearance switch re-derives the theme tokens, but a canvas colour we
  // set ourselves is a resolved palette entry that Qt will not update for us —
  // so re-apply it. ApplyChartPalette() returns early once the canvas already
  // carries the right colour, so the setPalette() inside it cannot recurse
  // through the QEvent::PaletteChange it emits.
  if (event->type() == QEvent::ApplicationPaletteChange ||
      event->type() == QEvent::PaletteChange) {
    ApplyChartPalette();
  }
}

void MetrixGraph::SetCanvasColor(const QColor& color) {
  canvas_color_overridden_ = true;

  QPalette pinned_palette = palette();
  pinned_palette.setColor(backgroundRole(), color);
  setPalette(pinned_palette);
}

void MetrixGraph::ApplyChartPalette() {
  // The base Graph canvas takes QPalette::Base, so by default the chart follows
  // the host OS appearance like any other data surface. An *explicit* theme
  // (Dark/Light/High contrast, as opposed to Theme::kSystem) is the authority
  // over the platform, so paint its `surface` token onto the background role
  // instead. background_color(), text_color() and grid_pen() all resolve from
  // palette().color(backgroundRole()) in the graph_qt base, so this single
  // override themes the fill, the axis text and the grid at once without
  // touching the submodule. Under kSystem the token is itself derived from the
  // live QPalette, which makes this a no-op.
  //
  // An operator-pinned canvas outranks both: they asked for that exact colour.
  if (canvas_color_overridden_)
    return;

  const scada::aui::ThemeTokens& tokens = ChartTokens();
  if (palette().color(backgroundRole()) == tokens.surface)
    return;

  QPalette themed_palette = palette();
  themed_palette.setColor(backgroundRole(), tokens.surface);
  setPalette(themed_palette);
}

#endif  // defined(UI_QT)

void MetrixGraph::UpdateCurBox() {
#if defined(UI_QT)
  // Update box for specified position of cursor line.
  for (auto* pane : panes()) {
    MetrixPane& matrix_pane = *static_cast<MetrixPane*>(pane);
    if (matrix_pane.plot().lines().empty())
      continue;

    if (matrix_pane.legend_) {
      matrix_pane.legend_->update();
    }
  }
#endif
}

void MetrixGraph::MetrixLine::UpdateTimeRange() {
  if (!data_source().connected())
    return;

  auto& graph_range = graph().horizontal_axis().range();

  auto from = scada::base::DecodeDoubleT(graph_range.low());
  auto to = graph().horizontal_axis().time_fit()
                ? kTimedDataCurrentOnly
                : scada::base::DecodeDoubleT(graph_range.high());

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
