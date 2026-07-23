#include "inspector/qt/inspector_panel.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "base/format_time.h"
#include "common/value_format.h"
#include "controller/selection_model.h"
#include "model/data_items_node_ids.h"
#include "model/node_id_util.h"
#include "modules/events/event_severity.h"
#include "modules/events/event_timeline.h"
#include "modules/inspector/limit_band.h"
#include "node_service/node_format.h"
#include "node_service/node_ref.h"
#include "scada/data_value.h"
#include "scada/node_id.h"
#include "scada/qualifier.h"
#include "timed_data/timed_data_spec.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <span>
#include <utility>

namespace {

// The design tokens for the active reshell theme (dark regardless when the
// panel is built standalone, e.g. for a capture).
const scada::aui::ThemeTokens& InspectorTokens() {
  return scada::aui::ActiveThemeTokens();
}

QString Tr(std::string_view text) {
  return QString::fromStdU16String(Translate(text));
}

// A small uppercase section header, matching the mockup's `.insec .h`.
QLabel* SectionHeader(const QString& text,
                      const scada::aui::ThemeTokens& tokens) {
  auto* label = new QLabel{text};
  label->setStyleSheet(
      QStringLiteral("color:%1;font-size:10px;font-weight:600;"
                     "text-transform:uppercase;letter-spacing:.5px;")
          .arg(tokens.fg_subtle.name()));
  return label;
}

// A "Key    Value" row, matching the mockup's `.kv`.
QWidget* KeyValueRow(const QString& key,
                     QLabel** value_out,
                     const scada::aui::ThemeTokens& tokens) {
  auto* row = new QWidget;
  auto* layout = new QHBoxLayout{row};
  layout->setContentsMargins(0, 3, 0, 3);
  auto* key_label = new QLabel{key};
  key_label->setStyleSheet(
      QStringLiteral("color:%1;").arg(tokens.fg_subtle.name()));
  auto* value_label = new QLabel;
  value_label->setStyleSheet(
      QStringLiteral("color:%1;font-weight:600;").arg(tokens.fg.name()));
  value_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  layout->addWidget(key_label);
  layout->addStretch(1);
  layout->addWidget(value_label);
  *value_out = value_label;
  return row;
}

// Reads the node's configured limit bands into Measurements rows, most severe
// first, marking the band the current value sits in. Bands the node does not
// configure are omitted, and a node carrying none renders no limits block at
// all. Limits are formatted through the node's own value format so they read
// like the value above them.
std::vector<InspectorLimitRow> MakeLimitRows(const NodeRef& node,
                                             const scada::DataValue& current) {
  if (!node)
    return {};

  namespace di = scada::data_items::id;
  const scada::Variant hihi = node[di::AnalogItemType_LimitHiHi].value();
  const scada::Variant hi = node[di::AnalogItemType_LimitHi].value();
  const scada::Variant lo = node[di::AnalogItemType_LimitLo].value();
  const scada::Variant lolo = node[di::AnalogItemType_LimitLoLo].value();

  const auto as_limit =
      [](const scada::Variant& value) -> std::optional<double> {
    if (value.is_null())
      return std::nullopt;
    return value.get_or(0.0);
  };
  const LimitValues limits{.lolo = as_limit(lolo),
                           .lo = as_limit(lo),
                           .hi = as_limit(hi),
                           .hihi = as_limit(hihi)};
  if (limits.empty())
    return {};

  const LimitBand band = current.value.is_null()
                             ? LimitBand::kNormal
                             : LimitBandFor(current.value.get_or(0.0), limits);

  std::vector<InspectorLimitRow> rows;
  const auto add = [&](std::string_view label, const scada::Variant& value,
                       LimitBand limit_band) {
    if (value.is_null())
      return;
    rows.push_back(
        {.label = Tr(label),
         .value = QString::fromStdU16String(FormatValue(node, value, {}, 0)),
         .breached = band == limit_band});
  };
  add("HiHi", hihi, LimitBand::kHiHi);
  add("Hi", hi, LimitBand::kHi);
  add("Lo", lo, LimitBand::kLo);
  add("LoLo", lolo, LimitBand::kLoLo);
  return rows;
}

}  // namespace

InspectorQualityBand InspectorQualityBandFor(
    const scada::Qualifier& qualifier) {
  return qualifier.bad() ? InspectorQualityBand::kBad
                         : InspectorQualityBand::kGood;
}

InspectorPanel::InspectorPanel(InspectorPanelContext context, QWidget* parent)
    : QWidget{parent}, context_{std::move(context)} {
  const scada::aui::ThemeTokens& tokens = InspectorTokens();
  setObjectName(QStringLiteral("inspectorPanel"));
  setStyleSheet(QStringLiteral("#inspectorPanel{background:%1;}")
                    .arg(tokens.bg_elevated.name()));

  auto* root = new QVBoxLayout{this};
  root->setContentsMargins(0, 0, 0, 0);

  stack_ = new QStackedWidget{this};
  stack_->setObjectName(QStringLiteral("inspectorStack"));
  stack_->addWidget(BuildEmptyState());   // index 0
  stack_->addWidget(BuildElementView());  // index 1
  stack_->addWidget(BuildEventView());    // index 2
  root->addWidget(stack_);

  Clear();
}

InspectorPanel::~InspectorPanel() = default;

QWidget* InspectorPanel::BuildEmptyState() {
  const scada::aui::ThemeTokens& tokens = InspectorTokens();
  auto* empty = new QWidget;
  auto* layout = new QVBoxLayout{empty};
  layout->setAlignment(Qt::AlignCenter);
  auto* label = new QLabel{Tr("Select an element to inspect it")};
  label->setWordWrap(true);
  label->setAlignment(Qt::AlignCenter);
  label->setStyleSheet(
      QStringLiteral("color:%1;padding:24px;").arg(tokens.fg_subtle.name()));
  layout->addWidget(label);
  return empty;
}

QWidget* InspectorPanel::BuildElementView() {
  const scada::aui::ThemeTokens& tokens = InspectorTokens();
  auto* view = new QWidget;
  auto* layout = new QVBoxLayout{view};
  layout->setContentsMargins(14, 14, 14, 14);
  layout->setSpacing(14);

  // Header: title + node-id subtitle.
  title_ = new QLabel;
  title_->setWordWrap(true);
  title_->setStyleSheet(
      QStringLiteral("color:%1;font-size:14px;font-weight:600;")
          .arg(tokens.fg.name()));
  subtitle_ = new QLabel;
  subtitle_->setObjectName(QStringLiteral("inspectorSubtitle"));
  subtitle_->setWordWrap(true);
  subtitle_->setStyleSheet(
      QStringLiteral("color:%1;font-size:11px;").arg(tokens.fg_subtle.name()));
  layout->addWidget(title_);
  layout->addWidget(subtitle_);

  // State hero: big value + quality pill.
  auto* hero = new QFrame;
  hero->setStyleSheet(
      QStringLiteral("background:%1;border:1px solid %2;border-radius:6px;")
          .arg(tokens.surface_muted.name(), tokens.border.name()));
  auto* hero_layout = new QHBoxLayout{hero};
  hero_layout->setContentsMargins(14, 12, 14, 12);
  value_ = new QLabel;
  value_->setObjectName(QStringLiteral("inspectorValue"));
  value_->setStyleSheet(
      QStringLiteral("color:%1;font-size:20px;font-weight:700;")
          .arg(tokens.fg.name()));
  quality_ = new QLabel;
  quality_->setObjectName(QStringLiteral("qualityPill"));
  hero_layout->addWidget(value_);
  hero_layout->addStretch(1);
  hero_layout->addWidget(quality_);
  layout->addWidget(hero);

  // Measurements section: the update time, then the node's configured limit
  // bands so the operator can see which threshold a coloured value crossed
  // (and how far the normal band reaches) without opening the limits editor.
  layout->addWidget(SectionHeader(Tr("Measurements"), tokens));
  layout->addWidget(KeyValueRow(Tr("Updated"), &updated_, tokens));

  limits_ = new QWidget;
  limits_->setObjectName(QStringLiteral("inspectorLimits"));
  auto* limits_layout = new QVBoxLayout{limits_};
  limits_layout->setContentsMargins(0, 0, 0, 0);
  limits_layout->setSpacing(0);
  limits_header_ = SectionHeader(Tr("Limits"), tokens);
  limits_layout->addWidget(limits_header_);
  layout->addWidget(limits_);

  // Control section.
  layout->addWidget(SectionHeader(Tr("Control"), tokens));
  control_ = new QPushButton{Tr("Control…")};
  control_->setObjectName(QStringLiteral("inspectorControl"));
  control_->setStyleSheet(
      QStringLiteral("QPushButton{background:%1;color:%2;border:none;"
                     "border-radius:6px;padding:8px;font-weight:600;}"
                     // A muted fill, not just muted text: a disabled control
                     // must not keep the accent's "actionable" colour.
                     "QPushButton:disabled{background:%4;color:%3;}")
          .arg(tokens.accent.name(), tokens.accent_fg.name(),
               tokens.fg_subtle.name(), tokens.surface_muted.name()));
  connect(control_, &QPushButton::clicked, this, [this] {
    if (context_.on_control)
      context_.on_control();
  });
  layout->addWidget(control_);
  auto* hint = new QLabel{
      Tr("Opens the two-stage command confirm. Actions are logged.")};
  hint->setWordWrap(true);
  hint->setStyleSheet(
      QStringLiteral("color:%1;font-size:11px;").arg(tokens.fg_subtle.name()));
  layout->addWidget(hint);

  // Why control is unavailable. A greyed button with no explanation leaves the
  // operator guessing whether the system is broken or they lack the right.
  control_reason_ = new QLabel;
  control_reason_->setObjectName(QStringLiteral("inspectorControlReason"));
  control_reason_->setWordWrap(true);
  control_reason_->setStyleSheet(
      QStringLiteral("color:%1;font-size:11px;").arg(tokens.fg_muted.name()));
  layout->addWidget(control_reason_);

  layout->addStretch(1);
  return view;
}

QWidget* InspectorPanel::BuildEventView() {
  const scada::aui::ThemeTokens& tokens = InspectorTokens();
  auto* view = new QWidget;
  auto* layout = new QVBoxLayout{view};
  layout->setContentsMargins(14, 14, 14, 14);
  layout->setSpacing(14);

  // Header: source identity.
  event_title_ = new QLabel;
  event_title_->setObjectName(QStringLiteral("inspectorEventTitle"));
  event_title_->setWordWrap(true);
  event_title_->setStyleSheet(
      QStringLiteral("color:%1;font-size:14px;font-weight:600;")
          .arg(tokens.fg.name()));
  event_subtitle_ = new QLabel;
  event_subtitle_->setWordWrap(true);
  event_subtitle_->setStyleSheet(
      QStringLiteral("color:%1;font-size:11px;").arg(tokens.fg_subtle.name()));
  layout->addWidget(event_title_);
  layout->addWidget(event_subtitle_);

  // Alarm hero: the severity band pill (colour + name + number — colour is
  // never the only signal) over the message.
  auto* hero = new QFrame;
  hero->setObjectName(QStringLiteral("inspectorEventHero"));
  // Scoped to the frame itself: an unscoped border rule would cascade onto
  // the child labels.
  hero->setStyleSheet(
      QStringLiteral("#inspectorEventHero{background:%1;border:1px solid %2;"
                     "border-radius:6px;}")
          .arg(tokens.surface_muted.name(), tokens.border.name()));
  auto* hero_layout = new QVBoxLayout{hero};
  hero_layout->setContentsMargins(14, 12, 14, 12);
  hero_layout->setSpacing(8);
  event_severity_ = new QLabel;
  event_severity_->setObjectName(QStringLiteral("inspectorEventSeverity"));
  hero_layout->addWidget(event_severity_, 0, Qt::AlignLeft);
  event_message_ = new QLabel;
  event_message_->setObjectName(QStringLiteral("inspectorEventMessage"));
  event_message_->setWordWrap(true);
  event_message_->setStyleSheet(
      QStringLiteral("color:%1;font-size:13px;font-weight:600;")
          .arg(tokens.fg.name()));
  hero_layout->addWidget(event_message_);
  layout->addWidget(hero);

  // Event details.
  layout->addWidget(SectionHeader(Tr("Event"), tokens));
  layout->addWidget(KeyValueRow(Tr("Time"), &event_time_, tokens));
  layout->addWidget(
      KeyValueRow(Tr("Acknowledged"), &event_acknowledged_, tokens));
  event_time_->setObjectName(QStringLiteral("inspectorEventTime"));
  event_acknowledged_->setObjectName(
      QStringLiteral("inspectorEventAcknowledged"));

  // Acknowledge action, through the journal's own command.
  acknowledge_ = new QPushButton{Tr("Acknowledge")};
  acknowledge_->setObjectName(QStringLiteral("inspectorAcknowledge"));
  acknowledge_->setStyleSheet(
      QStringLiteral("QPushButton{background:%1;color:%2;border:none;"
                     "border-radius:6px;padding:8px;font-weight:600;}"
                     "QPushButton:disabled{background:%3;color:%4;}")
          .arg(tokens.accent.name(), tokens.accent_fg.name(),
               tokens.surface_muted.name(), tokens.fg_subtle.name()));
  connect(acknowledge_, &QPushButton::clicked, this, [this] {
    if (context_.on_acknowledge)
      context_.on_acknowledge();
  });
  layout->addWidget(acknowledge_);

  // Go to source: opens the alarm's source element in a graph. Secondary
  // styling — Acknowledge keeps the accent as the card's primary action.
  go_to_source_ = new QPushButton{Tr("To graph")};
  go_to_source_->setObjectName(QStringLiteral("inspectorGoToSource"));
  go_to_source_->setStyleSheet(
      QStringLiteral("QPushButton{background:%1;color:%2;border:1px solid %3;"
                     "border-radius:6px;padding:8px;font-weight:600;}"
                     "QPushButton:disabled{color:%4;}")
          .arg(tokens.surface_muted.name(), tokens.fg.name(),
               tokens.border.name(QColor::HexArgb), tokens.fg_subtle.name()));
  connect(go_to_source_, &QPushButton::clicked, this, [this] {
    if (context_.on_go_to_source)
      context_.on_go_to_source();
  });
  layout->addWidget(go_to_source_);

  // History: the event's own lifecycle, so "when did this happen and has
  // anyone responded" is answered in the card rather than by reading the
  // journal's columns.
  timeline_ = new QWidget;
  timeline_->setObjectName(QStringLiteral("inspectorTimeline"));
  auto* timeline_layout = new QVBoxLayout{timeline_};
  timeline_layout->setContentsMargins(0, 0, 0, 0);
  timeline_layout->setSpacing(0);
  timeline_layout->addWidget(SectionHeader(Tr("History"), tokens));
  layout->addWidget(timeline_);

  layout->addStretch(1);
  return view;
}

void InspectorPanel::ShowTimeline(
    const std::vector<InspectorTimelineRow>& timeline) {
  const scada::aui::ThemeTokens& tokens = InspectorTokens();

  // Rebuild: the steps belong to the selected event, so they change with the
  // selection rather than with the value.
  auto* layout = qobject_cast<QVBoxLayout*>(timeline_->layout());
  while (layout->count() > 1) {
    QLayoutItem* item = layout->takeAt(1);
    delete item->widget();
    delete item;
  }

  timeline_->setVisible(!timeline.empty());
  if (timeline.empty())
    return;

  for (const InspectorTimelineRow& step : timeline) {
    auto* row = new QWidget;
    row->setObjectName(QStringLiteral("inspectorTimelineRow"));
    auto* row_layout = new QHBoxLayout{row};
    row_layout->setContentsMargins(0, 3, 0, 3);
    row_layout->setSpacing(9);

    // A step that has not happened yet shows an em dash where its time would
    // be, so the column stays aligned and the gap reads as "not yet".
    auto* time =
        new QLabel{step.time.isEmpty() ? QStringLiteral("—") : step.time};
    time->setObjectName(QStringLiteral("inspectorTimelineTime"));
    // Mono so the times form a readable column; nullopt under the legacy
    // look, where the panel is not built anyway.
    if (const std::optional<QFont> mono = scada::aui::MonoValueFont())
      time->setFont(*mono);
    time->setStyleSheet(QStringLiteral("color:%1;font-size:11px;")
                            .arg(tokens.fg_subtle.name()));
    auto* text = new QLabel{step.text};
    text->setWordWrap(true);
    text->setStyleSheet(
        QStringLiteral("color:%1;font-size:11px;").arg(tokens.fg_muted.name()));

    row_layout->addWidget(time);
    row_layout->addWidget(text, 1);
    layout->addWidget(row);
  }
}

void InspectorPanel::Clear() {
  spec_.reset();
  if (stack_)
    stack_->setCurrentIndex(0);
}

void InspectorPanel::ShowSelection(const SelectionModel& selection) {
  if (selection.empty() || selection.multiple()) {
    Clear();
    return;
  }

  // A journal-event selection shows the alarm card.
  if (const std::optional<scada::Event>& event = selection.event()) {
    spec_.reset();
    const NodeRef& source_node = selection.node();
    const QString source =
        source_node
            ? QString::fromStdU16String(ToString16(source_node.display_name()))
            : QString::fromStdString(
                  NodeIdToScadaString(event->source_node_id));
    // The lifecycle steps carry the time only: the card's Event section
    // already states the date, so repeating it on every row would crowd them.
    std::vector<InspectorTimelineRow> timeline;
    for (const events::EventTimelineEntry& entry :
         events::BuildEventTimeline(*event)) {
      timeline.push_back(
          {.time = scada::IsNull(entry.time) ? QString{}
                                        : QString::fromStdString(FormatTime(
                                              entry.time, TIME_FORMAT_TIME)),
           .text = Tr(events::EventTimelineStepText(entry.step))});
    }

    ShowEvent(InspectorEventView{
        .source = source,
        .node_id_text =
            QString::fromStdString(NodeIdToScadaString(event->source_node_id)),
        .message = QString::fromStdU16String(event->message.text),
        .severity = event->severity,
        .time_text = QString::fromStdString(
            FormatTime(event->time, TIME_FORMAT_DATE | TIME_FORMAT_TIME)),
        .acknowledged_text =
            event->acked ? QString::fromStdString(
                               FormatTime(event->acknowledged_time,
                                          TIME_FORMAT_DATE | TIME_FORMAT_TIME))
                         : QString::fromStdU16String(Translate("— pending —")),
        .acknowledgeable = !event->acked && context_.is_acknowledge_enabled &&
                           context_.is_acknowledge_enabled(),
        .source_available = context_.is_go_to_source_enabled &&
                            context_.is_go_to_source_enabled(),
        .timeline = std::move(timeline)});
    return;
  }

  const TimedDataSpec& source = selection.timed_data();
  if (source.node_id().is_null() && source.formula().empty()) {
    // A non-Variable node selection (a folder/object) carries no live value and
    // no data spec, so its title/value would render blank. Show the empty state
    // rather than an empty element card. A node-less spec with a formula (a
    // Table expression row) does carry a live value and is shown.
    Clear();
    return;
  }

  title_->setText(QString::fromStdU16String(selection.GetTitle()));

  // Copy the selection's connected spec (which shares the underlying live
  // TimedData) and own its update handler, so the readout keeps ticking
  // while this element stays selected.
  spec_ = std::make_unique<TimedDataSpec>(source);
  spec_->SetCurrentOnly();
  // Refresh on either delivery path. The live current value — the only thing a
  // SetCurrentOnly spec delivers — arrives as a PROPERTY_CURRENT change through
  // property_change_handler, not as a buffer update; wiring only update_handler
  // left the readout frozen at whatever it showed when the element was
  // selected, since a current-only spec produces no buffer updates.
  spec_->update_handler = [this](std::span<const scada::DataValue>) {
    RefreshValue();
  };
  spec_->property_change_handler = [this](const PropertySet&) {
    RefreshValue();
  };
  subtitle_->setText(QString::fromStdString(spec_->formula()));

  RefreshValue();
  stack_->setCurrentIndex(1);
}

void InspectorPanel::RefreshValue() {
  InspectorElementView element{.title = title_->text(),
                               .node_id_text = subtitle_->text()};
  if (spec_) {
    element.value_text = QString::fromStdU16String(
        spec_->GetCurrentString(ValueFormat{FORMAT_QUALITY | FORMAT_UNITS}));
    element.quality = InspectorQualityBandFor(spec_->current().qualifier);
    element.updated_text = QString::fromStdString(
        FormatTime(spec_->change_time(), TIME_FORMAT_TIME));
    element.limits = MakeLimitRows(spec_->node(), spec_->current());
  }

  element.controllable =
      context_.is_control_enabled && context_.is_control_enabled();
  // The host explains an unavailable control: it owns the command resolution
  // and the session, which the panel cannot see.
  if (!element.controllable && context_.control_reason)
    element.control_reason = context_.control_reason();

  ShowElement(element);
}

void InspectorPanel::ShowEvent(const InspectorEventView& event) {
  const scada::aui::ThemeTokens& tokens = InspectorTokens();

  event_title_->setText(event.source);
  event_subtitle_->setText(event.node_id_text);
  event_message_->setText(event.message.isEmpty() ? QStringLiteral("—")
                                                  : event.message);

  // The severity band pill: named + numbered, coloured from the shared ramp
  // (a routine severity reads as a neutral pill; colour is never the only
  // signal — the name and number carry the state).
  QString severity_text =
      QString::fromStdU16String(events::EventSeverityLabel(event.severity));
  if (!severity_text.isEmpty())
    severity_text += QLatin1Char(' ');
  severity_text += QString::number(event.severity);
  const std::optional<scada::aui::Color> band_color =
      scada::aui::SeverityColor(events::SeverityLevelForEvent(event.severity));
  const QColor pill = band_color ? band_color->qcolor() : tokens.fg_muted;
  event_severity_->setText(severity_text);
  event_severity_->setStyleSheet(
      QStringLiteral("#inspectorEventSeverity{color:%1;border:1px solid %1;"
                     "border-radius:9px;padding:1px 10px;font-weight:600;}")
          .arg(pill.name()));

  event_time_->setText(event.time_text.isEmpty() ? QStringLiteral("—")
                                                 : event.time_text);
  event_acknowledged_->setText(event.acknowledged_text.isEmpty()
                                   ? QStringLiteral("—")
                                   : event.acknowledged_text);
  acknowledge_->setEnabled(event.acknowledgeable);
  go_to_source_->setEnabled(event.source_available);

  ShowTimeline(event.timeline);

  stack_->setCurrentIndex(2);
}

void InspectorPanel::ShowElement(const InspectorElementView& element) {
  const scada::aui::ThemeTokens& tokens = InspectorTokens();

  title_->setText(element.title);
  subtitle_->setText(element.node_id_text);
  value_->setText(element.value_text.isEmpty() ? QStringLiteral("—")
                                               : element.value_text);

  const QColor pill =
      element.quality == InspectorQualityBand::kBad ? tokens.bad : tokens.good;
  quality_->setText(element.quality == InspectorQualityBand::kBad ? Tr("Bad")
                                                                  : Tr("Good"));
  quality_->setStyleSheet(
      QStringLiteral("#qualityPill{color:%1;border:1px solid %1;"
                     "border-radius:9px;padding:1px 10px;font-weight:600;}")
          .arg(pill.name()));

  updated_->setText(element.updated_text.isEmpty() ? QStringLiteral("—")
                                                   : element.updated_text);

  ShowLimits(element.limits);

  control_->setEnabled(element.controllable);
  control_reason_->setText(element.control_reason);
  control_reason_->setVisible(!element.controllable &&
                              !element.control_reason.isEmpty());

  stack_->setCurrentIndex(1);
}

void InspectorPanel::ShowLimits(const std::vector<InspectorLimitRow>& limits) {
  const scada::aui::ThemeTokens& tokens = InspectorTokens();

  // Rebuild the rows: the set of configured bands belongs to the node, so it
  // changes with the selection rather than with the value.
  auto* layout = qobject_cast<QVBoxLayout*>(limits_->layout());
  while (layout->count() > 1) {
    QLayoutItem* item = layout->takeAt(1);
    delete item->widget();
    delete item;
  }

  limits_->setVisible(!limits.empty());
  if (limits.empty())
    return;

  for (const InspectorLimitRow& limit : limits) {
    QLabel* value_label = nullptr;
    QWidget* row = KeyValueRow(limit.label, &value_label, tokens);
    // The breached band explains the value's colour, so it is named by its own
    // object name as well as coloured — colour is never the only signal.
    value_label->setObjectName(limit.breached
                                   ? QStringLiteral("inspectorLimitBreached")
                                   : QStringLiteral("inspectorLimitValue"));
    value_label->setText(limit.value);
    if (limit.breached) {
      value_label->setStyleSheet(
          QStringLiteral("color:%1;font-weight:700;").arg(tokens.bad.name()));
    }
    layout->addWidget(row);
  }
}
