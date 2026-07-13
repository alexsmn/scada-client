#include "display_frame/qt/display_frame.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "base/format_time.h"
#include "events/node_event_provider.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "scada/event.h"
#include "scada/node_id.h"
#include "timed_data/timed_data_service.h"
#include "timed_data/timed_data_spec.h"
#include "vds_runtime/qt/vds_runtime_widget.h"

#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
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

// The design tokens for the active reshell theme. The frame is only built under
// a token theme (WrapDisplayInFrame gates on it), so the legacy fallback here
// is harmless. Mirrors the BarTokens() helper in the event filter bar.
const scada::aui::ThemeTokens& FrameTokens() {
  scada::aui::Theme theme = scada::aui::Theme::kDark;
  switch (scada::aui::GetSeverityTheme()) {
    case scada::aui::SeverityTheme::kLight:
      theme = scada::aui::Theme::kLight;
      break;
    case scada::aui::SeverityTheme::kHighContrast:
      theme = scada::aui::Theme::kHighContrast;
      break;
    default:
      break;
  }
  return scada::aui::GetThemeTokens(theme);
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

QString SeverityLabel(unsigned severity) {
  switch (DisplaySeverityBandFor(severity)) {
    case DisplaySeverityBand::kCritical:
      return Tr("Critical");
    case DisplaySeverityBand::kWarning:
      return Tr("Warning");
    case DisplaySeverityBand::kInfo:
      break;
  }
  return Tr("Info");
}

QColor SeverityColor(const scada::aui::ThemeTokens& tokens, unsigned severity) {
  switch (DisplaySeverityBandFor(severity)) {
    case DisplaySeverityBand::kCritical:
      return tokens.severity_critical;
    case DisplaySeverityBand::kWarning:
      return tokens.severity_medium;
    case DisplaySeverityBand::kInfo:
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

DisplaySeverityBand DisplaySeverityBandFor(unsigned severity) {
  if (severity >= scada::kSeverityCritical)
    return DisplaySeverityBand::kCritical;
  if (severity >= scada::kSeverityWarning)
    return DisplaySeverityBand::kWarning;
  return DisplaySeverityBand::kInfo;
}

DisplayFrame::DisplayFrame(VdsRuntimeWidget* diagram,
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

  spec_ptr->update_handler = [this,
                              spec_ptr](std::span<const scada::DataValue>) {
    for (std::size_t i = 0; i < measurement_specs_.size(); ++i) {
      if (measurement_specs_[i].get() == spec_ptr) {
        RefreshMeasurementRow(static_cast<int>(i), *spec_ptr);
        break;
      }
    }
  };

  RefreshMeasurementRow(row, *spec_ptr);
}

void DisplayFrame::RefreshMeasurementRow(int row, const TimedDataSpec& spec) {
  if (!measurements_ || row < 0 || row >= measurements_->rowCount())
    return;

  measurements_->setItem(
      row, 0, new QTableWidgetItem{QString::fromStdU16String(spec.GetTitle())});
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
      NodeRef node = data_context_.node_service->GetNode(event.node_id);
      if (node)
        object = QString::fromStdU16String(node.display_name());
    }
    events_->setItem(i, 2, new QTableWidgetItem{object});

    events_->setItem(
        i, 3, new QTableWidgetItem{QString::fromStdU16String(event.message)});
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
      event->type() == QEvent::Resize && fit_) {
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

QWidget* WrapDisplayInFrame(VdsRuntimeWidget* diagram,
                            QString breadcrumb,
                            DisplayFrameContext data_context) {
  if (scada::aui::GetSeverityTheme() == scada::aui::SeverityTheme::kLegacy)
    return diagram;
  return new DisplayFrame{diagram, std::move(breadcrumb), data_context};
}
