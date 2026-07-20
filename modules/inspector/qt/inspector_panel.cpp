#include "inspector/qt/inspector_panel.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "base/format_time.h"
#include "common/value_format.h"
#include "controller/selection_model.h"
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

// The design tokens for the active reshell theme. The panel is only built under
// a token theme (the host gates on it), so the legacy fallback is harmless.
const scada::aui::ThemeTokens& InspectorTokens() {
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

  // Measurements section.
  layout->addWidget(SectionHeader(Tr("Measurements"), tokens));
  layout->addWidget(KeyValueRow(Tr("Updated"), &updated_, tokens));

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

  layout->addStretch(1);
  return view;
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
  spec_->update_handler = [this](std::span<const scada::DataValue>) {
    RefreshValue();
  };
  subtitle_->setText(QString::fromStdString(spec_->formula()));

  RefreshValue();
  stack_->setCurrentIndex(1);
}

void InspectorPanel::RefreshValue() {
  QString value_text;
  InspectorQualityBand band = InspectorQualityBand::kGood;
  QString updated_text;
  if (spec_) {
    value_text = QString::fromStdU16String(
        spec_->GetCurrentString(ValueFormat{FORMAT_QUALITY | FORMAT_UNITS}));
    band = InspectorQualityBandFor(spec_->current().qualifier);
    updated_text = QString::fromStdString(
        FormatTime(spec_->change_time(), TIME_FORMAT_TIME));
  }

  const bool controllable =
      context_.is_control_enabled && context_.is_control_enabled();

  ShowElement(title_->text(), subtitle_->text(), value_text, band, updated_text,
              controllable);
}

void InspectorPanel::ShowElement(const QString& title,
                                 const QString& node_id_text,
                                 const QString& value_text,
                                 InspectorQualityBand quality,
                                 const QString& updated_text,
                                 bool controllable) {
  const scada::aui::ThemeTokens& tokens = InspectorTokens();

  title_->setText(title);
  subtitle_->setText(node_id_text);
  value_->setText(value_text.isEmpty() ? QStringLiteral("—") : value_text);

  const QColor pill =
      quality == InspectorQualityBand::kBad ? tokens.bad : tokens.good;
  quality_->setText(quality == InspectorQualityBand::kBad ? Tr("Bad")
                                                          : Tr("Good"));
  quality_->setStyleSheet(
      QStringLiteral("#qualityPill{color:%1;border:1px solid %1;"
                     "border-radius:9px;padding:1px 10px;font-weight:600;}")
          .arg(pill.name()));

  updated_->setText(updated_text.isEmpty() ? QStringLiteral("—")
                                           : updated_text);

  control_->setEnabled(controllable);

  stack_->setCurrentIndex(1);
}
