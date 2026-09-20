#include "display_frame/qt/display_frame.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "base/format_time.h"
#include "display_view/qt/display_widget.h"
#include "events/event_severity.h"
#include "events/node_event_provider.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "scada/event.h"
#include "scada/node_id.h"
#include "timed_data/timed_data_service.h"
#include "timed_data/timed_data_spec.h"

#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSplitter>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

constexpr double kMinZoom = 0.05;
constexpr double kMaxZoom = 8.0;
constexpr double kZoomStep = 1.25;

// Most recent events kept in the Recent-events strip.
constexpr int kMaxRecentEvents = 50;

// The legend's inset from the viewport's bottom-left corner, from the mockup's
// `.legend` rule (left: 14px; bottom: 12px).
constexpr int kLegendMarginLeft = 14;
constexpr int kLegendMarginBottom = 12;

// One legend swatch, from the mockup's `.legend .sw`.
constexpr int kLegendSwatchSize = 12;

// The design tokens for the active theme. The frame is only built under
// a token theme (WrapDisplayInFrame gates on it), so the legacy fallback here
// is harmless. Mirrors the BarTokens() helper in the event filter bar.
const scada::aui::ThemeTokens& FrameTokens() {
  return scada::aui::ActiveThemeTokens();
}

// A translucent "soft" tint of `color` for pill / chip fills, matching the
// mockup's `--good-soft` style (the token set exposes only the solid colours).
QString SoftRgba(const QColor& color, double alpha) {
  return QStringLiteral("rgba(%1,%2,%3,%4)")
      .arg(color.red())
      .arg(color.green())
      .arg(color.blue())
      .arg(alpha, 0, 'f', 2);
}

QString Tr(std::string_view text) {
  return QString::fromStdU16String(Translate(text));
}

// How one legend swatch is drawn. Shape as well as colour, because
// design-language.md §2 requires a single-line state to stay legible without
// relying on hue: `kFilled` is a closed device, `kHollow` an open one or a
// conductor, and `kSelection` is a miniature of the halo the diagram paints.
enum class LegendSwatchKind { kFilled, kHollow, kSelection };

// The legend's colour chip, painted rather than styled.
//
// A QSS `border: 1.5px dashed` with a `border-radius` renders SOLID at this
// size -- measured on the generated capture, where the Selected swatch came
// out an unbroken outline and was left distinguishable from Open and
// Energized by hue alone. Painting it is also what lets the selection chip
// carry the same inner-plus-dashed-outer pair the halo does, so the legend
// reads as a key to the drawing rather than as a separate vocabulary.
class LegendSwatch final : public QWidget {
 public:
  LegendSwatch(const QColor& color, LegendSwatchKind kind, QWidget* parent)
      : QWidget{parent}, color_{color}, kind_{kind} {
    setFixedSize(kLegendSwatchSize, kLegendSwatchSize);
  }

 protected:
  void paintEvent(QPaintEvent*) override {
    QPainter painter{this};
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (kind_ == LegendSwatchKind::kSelection) {
      // The halo, in miniature: a solid rect on the symbol and a dashed one
      // outside it. Same order and same relative weights as PaintSelection.
      const QRectF inner = QRectF{rect()}.adjusted(3.5, 3.5, -3.5, -3.5);
      painter.setBrush(Qt::NoBrush);
      painter.setPen(QPen{color_, 1.5});
      painter.drawRect(inner);

      QPen outer{color_, 1.0, Qt::DashLine};
      outer.setDashPattern({2, 2});
      painter.setPen(outer);
      painter.setOpacity(0.8);
      painter.drawRect(QRectF{rect()}.adjusted(0.5, 0.5, -0.5, -0.5));
      return;
    }

    const QRectF box = QRectF{rect()}.adjusted(0.75, 0.75, -0.75, -0.75);
    painter.setBrush(kind_ == LegendSwatchKind::kFilled ? QBrush{color_}
                                                        : Qt::NoBrush);
    painter.setPen(QPen{color_, 1.5});
    painter.drawRoundedRect(box, 3, 3);
  }

 private:
  QColor color_;
  LegendSwatchKind kind_;
};

// One legend entry: a swatch in `color` followed by `label`.
QWidget* MakeLegendEntry(const QString& label,
                         const QColor& color,
                         LegendSwatchKind kind,
                         const scada::aui::ThemeTokens& tokens,
                         QWidget* parent) {
  auto* entry = new QWidget{parent};
  auto* layout = new QHBoxLayout{entry};
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(6);

  layout->addWidget(new LegendSwatch{color, kind, entry});

  auto* text = new QLabel{label, entry};
  text->setStyleSheet(QStringLiteral("color:%1;").arg(tokens.fg_muted.name()));
  layout->addWidget(text);

  return entry;
}

QString SeverityLabel(unsigned severity) {
  // `events::SeverityLevelLabel` is not used here: it answers with an empty
  // string for kNone, and this strip has a column to fill, so a routine event
  // is named "Info" rather than left blank.
  switch (events::SeverityLevelForEvent(severity)) {
    case scada::aui::SeverityLevel::kCritical:
      return Tr("Critical");
    case scada::aui::SeverityLevel::kWarning:
      return Tr("Warning");
    case scada::aui::SeverityLevel::kNone:
      break;
  }
  return Tr("Info");
}

QColor SeverityColor(const scada::aui::ThemeTokens& tokens, unsigned severity) {
  switch (events::SeverityLevelForEvent(severity)) {
    case scada::aui::SeverityLevel::kCritical:
      return tokens.severity_critical;
    case scada::aui::SeverityLevel::kWarning:
      return tokens.severity_medium;
    case scada::aui::SeverityLevel::kNone:
      break;
  }
  return tokens.severity_low;
}

// Builds a bay-strip panel — a header label above a read-only table — matching
// the mockup's `.bpanel`. Returns the panel widget; `*out_table` receives the
// table for population.
QWidget* MakeStripPanel(const QString& title,
                        const QStringList& columns,
                        const scada::aui::ThemeTokens& tokens,
                        QTableWidget** out_table) {
  auto* panel = new QWidget;
  auto* layout = new QVBoxLayout{panel};
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  auto* header = new QLabel{title, panel};
  header->setStyleSheet(
      QStringLiteral("background:%1;color:%2;padding:5px 12px;font-weight:600;"
                     "border-bottom:1px solid %3;")
          .arg(tokens.surface.name(), tokens.fg.name(), tokens.border.name()));
  layout->addWidget(header);

  auto* table = new QTableWidget{0, static_cast<int>(columns.size()), panel};
  table->setHorizontalHeaderLabels(columns);
  table->verticalHeader()->setVisible(false);
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table->setSelectionBehavior(QAbstractItemView::SelectRows);
  table->setSelectionMode(QAbstractItemView::SingleSelection);
  table->setShowGrid(false);
  table->setWordWrap(false);
  table->horizontalHeader()->setStretchLastSection(true);
  table->horizontalHeader()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
  table->setStyleSheet(
      QStringLiteral(
          "QTableWidget{background:%1;color:%2;border:none;gridline-color:%3;}"
          "QHeaderView::section{background:%4;color:%5;border:none;"
          "border-bottom:1px solid %3;padding:3px 10px;}"
          "QTableWidget::item{padding:2px 10px;border-bottom:1px solid %3;}")
          .arg(tokens.surface.name(), tokens.fg_muted.name(),
               tokens.border.name(), tokens.surface_muted.name(),
               tokens.fg_subtle.name()));
  layout->addWidget(table);

  *out_table = table;
  return panel;
}

}  // namespace

double ClampDisplayZoom(double zoom) {
  if (!std::isfinite(zoom))
    return 1.0;
  return std::clamp(zoom, kMinZoom, kMaxZoom);
}

double DisplayFitFactor(QSize natural, QSize viewport) {
  if (natural.width() <= 0 || natural.height() <= 0 || viewport.width() <= 0 ||
      viewport.height() <= 0) {
    return 1.0;
  }
  const double fx = static_cast<double>(viewport.width()) / natural.width();
  const double fy = static_cast<double>(viewport.height()) / natural.height();
  return ClampDisplayZoom(std::min(fx, fy));
}

int DisplayZoomPercent(double zoom) {
  return static_cast<int>(std::lround(ClampDisplayZoom(zoom) * 100.0));
}

QPoint DisplayLegendOrigin(QSize legend, QSize viewport) {
  // Clamped at the top: a viewport shorter than the legend would otherwise
  // place it at a negative y, hiding the entries the operator most needs. It
  // overlaps the diagram in that case, which is the lesser loss.
  return {
      kLegendMarginLeft,
      std::max(viewport.height() - legend.height() - kLegendMarginBottom, 0)};
}

DisplayFrame::DisplayFrame(DisplayWidget* diagram,
                           QString breadcrumb,
                           DisplayFrameContext data_context,
                           QWidget* parent)
    : QWidget{parent}, data_context_{data_context}, diagram_{diagram} {
  auto* root = new QVBoxLayout{this};
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(0);

  BuildToolbar(breadcrumb);

  scroll_ = new QScrollArea{this};
  scroll_->setWidgetResizable(false);
  scroll_->setAlignment(Qt::AlignCenter);
  scroll_->setFrameShape(QFrame::NoFrame);
  if (diagram_)
    scroll_->setWidget(diagram_);
  root->addWidget(scroll_, /*stretch=*/1);

  const scada::aui::ThemeTokens& tokens = FrameTokens();
  scroll_->viewport()->setStyleSheet(
      QStringLiteral("background:%1;").arg(tokens.bg.name()));

  BuildLegend();

  if (QWidget* strips = BuildBayStrips())
    root->addWidget(strips);

  // Refit when the viewport is (re)sized — it gets its real size only after the
  // frame's layout runs, which is after the frame's own resizeEvent.
  scroll_->viewport()->installEventFilter(this);

  if (data_context_.node_event_provider) {
    data_context_.node_event_provider->AddObserver(*this);
    RefreshEvents();
  }

  ApplyZoom();
}

DisplayFrame::~DisplayFrame() {
  if (data_context_.node_event_provider)
    data_context_.node_event_provider->RemoveObserver(*this);
}

void DisplayFrame::BuildToolbar(const QString& breadcrumb) {
  const scada::aui::ThemeTokens& tokens = FrameTokens();

  auto* bar = new QWidget{this};
  bar->setObjectName(QStringLiteral("displayToolbar"));
  bar->setStyleSheet(
      QStringLiteral(
          "#displayToolbar{background:%1;border-bottom:1px solid %2;}"
          "#displayToolbar QToolButton{color:%3;border:none;padding:2px 8px;}"
          "#displayToolbar QToolButton:hover{color:%4;}"
          "#displayToolbar QLabel{color:%3;}")
          .arg(tokens.bg_elevated.name(), tokens.border.name(),
               tokens.fg_muted.name(), tokens.fg.name()));

  auto* layout = new QHBoxLayout{bar};
  layout->setContentsMargins(12, 4, 12, 4);
  layout->setSpacing(10);

  // Hotspot breadcrumb — the display's location / title.
  if (!breadcrumb.isEmpty()) {
    auto* crumb = new QLabel{breadcrumb, bar};
    crumb->setStyleSheet(
        QStringLiteral("color:%1;font-weight:600;").arg(tokens.fg.name()));
    layout->addWidget(crumb);
  }

  // Live indicator pill.
  auto* live = new QLabel{Tr("Live"), bar};
  live->setObjectName(QStringLiteral("liveIndicator"));
  live->setStyleSheet(
      QStringLiteral("#liveIndicator{background:%1;color:%2;border-radius:9px;"
                     "padding:1px 10px;font-weight:600;}")
          .arg(SoftRgba(tokens.good, 0.16), tokens.good.name()));
  layout->addWidget(live);

  layout->addStretch(1);

  // Zoom / fit / 100% controls.
  auto* zoom_out = new QToolButton{bar};
  zoom_out->setText(QStringLiteral("-"));
  zoom_out->setToolTip(Tr("Zoom out"));
  connect(zoom_out, &QToolButton::clicked, this,
          [this] { SetZoom(zoom_ / kZoomStep); });
  layout->addWidget(zoom_out);

  zoom_label_ = new QLabel{QStringLiteral("100%"), bar};
  zoom_label_->setMinimumWidth(40);
  zoom_label_->setAlignment(Qt::AlignCenter);
  layout->addWidget(zoom_label_);

  auto* zoom_in = new QToolButton{bar};
  zoom_in->setText(QStringLiteral("+"));
  zoom_in->setToolTip(Tr("Zoom in"));
  connect(zoom_in, &QToolButton::clicked, this,
          [this] { SetZoom(zoom_ * kZoomStep); });
  layout->addWidget(zoom_in);

  auto* fit = new QToolButton{bar};
  fit->setText(Tr("Fit"));
  fit->setToolTip(Tr("Fit to window"));
  connect(fit, &QToolButton::clicked, this, [this] {
    fit_ = true;
    RefitToViewport();
  });
  layout->addWidget(fit);

  auto* reset = new QToolButton{bar};
  reset->setText(QStringLiteral("100%"));
  reset->setToolTip(Tr("Actual size"));
  connect(reset, &QToolButton::clicked, this, [this] { SetZoom(1.0); });
  layout->addWidget(reset);

  auto* export_button = new QToolButton{bar};
  export_button->setText(Tr("Export"));
  export_button->setToolTip(Tr("Export image"));
  connect(export_button, &QToolButton::clicked, this,
          [this] { ExportImage(); });
  layout->addWidget(export_button);

  qobject_cast<QVBoxLayout*>(this->layout())->insertWidget(0, bar);
}

void DisplayFrame::BuildLegend() {
  if (!scroll_)
    return;

  const scada::aui::ThemeTokens& tokens = FrameTokens();

  // Parented to the VIEWPORT, not to the diagram: the diagram is resized by
  // zoom and scrolls under the viewport, and a legend that scrolled away with
  // it would stop being a legend. The mockup pins it to the diagram pane.
  legend_ = new QWidget{scroll_->viewport()};
  legend_->setObjectName(QStringLiteral("displayLegend"));
  legend_->setStyleSheet(
      QStringLiteral("#displayLegend{background:%1;border:1px solid %2;"
                     "border-radius:8px;}")
          .arg(SoftRgba(tokens.surface, 0.88), tokens.border.name()));

  auto* layout = new QHBoxLayout{legend_};
  layout->setContentsMargins(10, 6, 10, 6);
  layout->setSpacing(14);

  // The four states the mockup legends, in its order. The colours are the
  // single-line tokens the renderer itself colours with, so the legend cannot
  // drift from the diagram; `accent` is the selection halo's own colour.
  //
  // The mockup labels the second entry "Open" and this says "Open / not in
  // service" instead -- deliberately, not as a wording preference.
  // `Translate()` has no context to disambiguate with, and the empty context
  // already maps "Open" to «Открыть», the FILE action; «Открыть» beside a
  // breaker symbol is worse than an untranslated label, and no check would
  // have reported it, because the string HAS a translation -- just not this
  // meaning. "Closed" is spelled out for the same reason (it is already
  // «Закрыт»). See client/CLAUDE.md, Localization.
  layout->addWidget(MakeLegendEntry(Tr("Closed / in service"), tokens.sl_closed,
                                    LegendSwatchKind::kFilled, tokens,
                                    legend_));
  layout->addWidget(MakeLegendEntry(Tr("Open / not in service"), tokens.sl_open,
                                    LegendSwatchKind::kHollow, tokens,
                                    legend_));
  layout->addWidget(MakeLegendEntry(Tr("Energized"), tokens.sl_live,
                                    LegendSwatchKind::kHollow, tokens,
                                    legend_));
  layout->addWidget(MakeLegendEntry(Tr("Selected"), tokens.accent,
                                    LegendSwatchKind::kSelection, tokens,
                                    legend_));

  legend_->adjustSize();
  legend_->raise();
  PlaceLegend();
}

void DisplayFrame::PlaceLegend() {
  if (!legend_ || !scroll_)
    return;
  legend_->adjustSize();
  legend_->move(
      DisplayLegendOrigin(legend_->size(), scroll_->viewport()->size()));
}

QWidget* DisplayFrame::BuildBayStrips() {
  const bool want_measurements = data_context_.timed_data_service != nullptr;
  const bool want_events = data_context_.node_event_provider != nullptr;
  if (!want_measurements && !want_events)
    return nullptr;

  const scada::aui::ThemeTokens& tokens = FrameTokens();

  auto* splitter = new QSplitter{Qt::Horizontal, this};
  splitter->setChildrenCollapsible(false);
  splitter->setFixedHeight(190);
  splitter->setStyleSheet(
      QStringLiteral("QSplitter{background:%1;border-top:1px solid %2;}"
                     "QSplitter::handle{background:%2;}")
          .arg(tokens.border.name(), tokens.border.name()));

  if (want_measurements) {
    splitter->addWidget(MakeStripPanel(
        Tr("Measurements"), {Tr("Signal"), Tr("Value"), Tr("Updated")}, tokens,
        &measurements_));
    measurements_->setObjectName(QStringLiteral("displayMeasurements"));
  }
  if (want_events) {
    splitter->addWidget(MakeStripPanel(
        Tr("Recent events"),
        {Tr("Severity"), Tr("Time"), Tr("Object"), Tr("Message")}, tokens,
        &events_));
  }
  return splitter;
}

void DisplayFrame::ShowMeasurement(const scada::NodeId& node_id) {
  if (!measurements_ || !data_context_.timed_data_service)
    return;

  // Already watched — nothing to add (append-only keeps row indices stable).
  for (const auto& spec : measurement_specs_) {
    if (spec->node_id() == node_id)
      return;
  }

  auto spec = std::make_unique<TimedDataSpec>(*data_context_.timed_data_service,
                                              node_id);
  spec->SetCurrentOnly();
  TimedDataSpec* spec_ptr = spec.get();

  const int row = measurements_->rowCount();
  measurements_->insertRow(row);
  measurement_specs_.push_back(std::move(spec));

  // Refresh the row when its data changes. The current value — the only thing
  // a SetCurrentOnly spec ever delivers — arrives as a PROPERTY_CURRENT change
  // through property_change_handler, not as a buffer update; wiring only
  // update_handler (as this once did) left the row blank forever, because
  // there are no buffer updates in current-only mode. Both are wired so the
  // strip also stays live if a ranged spec is ever used here.
  const auto refresh = [this, spec_ptr] {
    for (std::size_t i = 0; i < measurement_specs_.size(); ++i) {
      if (measurement_specs_[i].get() == spec_ptr) {
        RefreshMeasurementRow(static_cast<int>(i), *spec_ptr);
        break;
      }
    }
  };
  spec_ptr->update_handler = [refresh](std::span<const scada::DataValue>) {
    refresh();
  };
  spec_ptr->property_change_handler = [refresh](const PropertySet&) {
    refresh();
  };

  RefreshMeasurementRow(row, *spec_ptr);
}

void DisplayFrame::RefreshMeasurementRow(int row, const TimedDataSpec& spec) {
  if (!measurements_ || row < 0 || row >= measurements_->rowCount())
    return;

  measurements_->setItem(
      row, 0,
      new QTableWidgetItem{QString::fromStdU16String(spec.GetTitle().text)});
  // Value carries quality + units inline (the token set has no per-cell quality
  // colour column here; the string is the operator-facing value).
  measurements_->setItem(
      row, 1,
      new QTableWidgetItem{QString::fromStdU16String(spec.GetCurrentString())});
  measurements_->setItem(row, 2,
                         new QTableWidgetItem{QString::fromStdString(FormatTime(
                             spec.change_time(), TIME_FORMAT_TIME))});
}

void DisplayFrame::RefreshEvents() {
  if (!events_ || !data_context_.node_event_provider)
    return;

  std::vector<const scada::Event*> recent;
  for (const auto& [id, event] :
       data_context_.node_event_provider->unacked_events()) {
    recent.push_back(&event);
  }
  // Most recent first.
  std::sort(recent.begin(), recent.end(),
            [](const scada::Event* a, const scada::Event* b) {
              return a->time > b->time;
            });
  if (recent.size() > static_cast<std::size_t>(kMaxRecentEvents))
    recent.resize(kMaxRecentEvents);

  const scada::aui::ThemeTokens& tokens = FrameTokens();
  events_->setRowCount(static_cast<int>(recent.size()));
  for (int i = 0; i < static_cast<int>(recent.size()); ++i) {
    const scada::Event& event = *recent[i];

    auto* severity = new QTableWidgetItem{SeverityLabel(event.severity)};
    severity->setForeground(SeverityColor(tokens, event.severity));
    events_->setItem(i, 0, severity);

    events_->setItem(i, 1,
                     new QTableWidgetItem{QString::fromStdString(
                         FormatTime(event.time, TIME_FORMAT_TIME))});

    QString object;
    if (data_context_.node_service) {
      NodeRef node = data_context_.node_service->GetNode(event.source_node_id);
      if (node)
        object = QString::fromStdU16String(ToString16(node.display_name()));
    }
    events_->setItem(i, 2, new QTableWidgetItem{object});

    events_->setItem(
        i, 3,
        new QTableWidgetItem{QString::fromStdU16String(event.message.text)});
  }
}

void DisplayFrame::OnEvents(std::span<const scada::Event* const>) {
  RefreshEvents();
}

void DisplayFrame::OnAllEventsAcknowledged() {
  RefreshEvents();
}

QSize DisplayFrame::DiagramNaturalSize() const {
  if (!diagram_)
    return {640, 480};
  QSize hint = diagram_->sizeHint();
  return {std::max(hint.width(), 1), std::max(hint.height(), 1)};
}

void DisplayFrame::ApplyZoom() {
  if (!diagram_)
    return;
  const QSize natural = DiagramNaturalSize();
  diagram_->setFixedSize(
      QSize{static_cast<int>(std::lround(natural.width() * zoom_)),
            static_cast<int>(std::lround(natural.height() * zoom_))});
  if (zoom_label_)
    zoom_label_->setText(QStringLiteral("%1%").arg(DisplayZoomPercent(zoom_)));
}

void DisplayFrame::RefitToViewport() {
  if (!scroll_)
    return;
  zoom_ = DisplayFitFactor(DiagramNaturalSize(), scroll_->viewport()->size());
  ApplyZoom();
}

void DisplayFrame::SetZoom(double zoom) {
  fit_ = false;
  zoom_ = ClampDisplayZoom(zoom);
  ApplyZoom();
}

bool DisplayFrame::eventFilter(QObject* watched, QEvent* event) {
  if (scroll_ && watched == scroll_->viewport() &&
      event->type() == QEvent::Resize) {
    // Unconditionally, unlike the refit: the legend is anchored to the
    // viewport's bottom edge whether or not the page is being kept fitted, so
    // gating it on `fit_` would strand it the moment the operator zoomed.
    PlaceLegend();
    if (fit_)
      RefitToViewport();
  }
  return QWidget::eventFilter(watched, event);
}

void DisplayFrame::ExportImage() {
  if (!diagram_)
    return;
  const QString path = QFileDialog::getSaveFileName(
      this, Tr("Export image"), QString{}, QStringLiteral("PNG (*.png)"));
  if (path.isEmpty())
    return;
  diagram_->grab().save(path);
}

QWidget* WrapDisplayInFrame(DisplayWidget* diagram,
                            QString breadcrumb,
                            DisplayFrameContext data_context) {
  return new DisplayFrame{diagram, std::move(breadcrumb), data_context};
}
