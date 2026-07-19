#include "transmission_rules/qt/transmission_rule_inspector.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "model/devices_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_util.h"
#include "scada/basic_types.h"
#include "scada/variant.h"
#include "transmission_rules/transmission_rule.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <string_view>
#include <utility>

namespace {

const scada::aui::ThemeTokens& PanelTokens() {
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

// A muted uppercase section header, matching the users-admin panel.
QLabel* MakeSectionHeader(const QString& text,
                          const scada::aui::ThemeTokens& tokens) {
  auto* label = new QLabel{text};
  label->setStyleSheet(
      QStringLiteral("color:%1;font-size:10px;font-weight:600;"
                     "text-transform:uppercase;letter-spacing:.5px;")
          .arg(tokens.fg_subtle.name()));
  return label;
}

}  // namespace

TransmissionRuleInspector::TransmissionRuleInspector(QWidget* parent)
    : QWidget{parent} {
  const scada::aui::ThemeTokens& tokens = PanelTokens();
  setObjectName(QStringLiteral("transmissionRuleInspector"));
  setStyleSheet(QStringLiteral("#transmissionRuleInspector{background:%1;}")
                    .arg(tokens.bg_elevated.name()));

  auto* root = new QVBoxLayout{this};
  root->setContentsMargins(0, 0, 0, 0);

  stack_ = new QStackedWidget{this};
  stack_->addWidget(BuildEmptyState());  // index 0
  stack_->addWidget(BuildContent());     // index 1
  root->addWidget(stack_);

  Clear();
}

TransmissionRuleInspector::~TransmissionRuleInspector() = default;

void TransmissionRuleInspector::SetApplyHandler(ApplyHandler handler) {
  apply_handler_ = std::move(handler);
}

QWidget* TransmissionRuleInspector::BuildEmptyState() {
  const scada::aui::ThemeTokens& tokens = PanelTokens();
  auto* empty = new QWidget;
  auto* layout = new QVBoxLayout{empty};
  layout->setAlignment(Qt::AlignCenter);
  auto* label = new QLabel{Tr("Select a transmission rule to edit it")};
  label->setWordWrap(true);
  label->setAlignment(Qt::AlignCenter);
  label->setStyleSheet(
      QStringLiteral("color:%1;padding:24px;").arg(tokens.fg_subtle.name()));
  layout->addWidget(label);
  return empty;
}

QWidget* TransmissionRuleInspector::BuildContent() {
  const scada::aui::ThemeTokens& tokens = PanelTokens();
  auto* view = new QWidget;
  auto* layout = new QVBoxLayout{view};
  layout->setContentsMargins(14, 14, 14, 14);
  layout->setSpacing(10);

  // Header: rule summary + protocol subtitle.
  summary_ = new QLabel;
  summary_->setWordWrap(true);
  summary_->setStyleSheet(
      QStringLiteral("color:%1;font-size:14px;font-weight:600;")
          .arg(tokens.fg.name()));
  layout->addWidget(summary_);

  protocol_ = new QLabel;
  protocol_->setStyleSheet(
      QStringLiteral("color:%1;font-size:11px;").arg(tokens.fg_muted.name()));
  layout->addWidget(protocol_);

  // Source section: signal name + type tag pill.
  layout->addWidget(MakeSectionHeader(Tr("Source"), tokens));
  auto* source_row = new QHBoxLayout;
  source_ = new QLabel;
  source_->setWordWrap(true);
  source_->setStyleSheet(QStringLiteral("color:%1;").arg(tokens.fg.name()));
  signal_tag_ = new QLabel;
  signal_tag_->setObjectName(QStringLiteral("signalTagPill"));
  source_row->addWidget(source_);
  source_row->addStretch(1);
  source_row->addWidget(signal_tag_);
  layout->addLayout(source_row);

  // Destination section: endpoint (read-only) + IOA (editable).
  layout->addWidget(MakeSectionHeader(Tr("Destination"), tokens));
  endpoint_ = new QLabel;
  endpoint_->setWordWrap(true);
  endpoint_->setStyleSheet(
      QStringLiteral("color:%1;").arg(tokens.fg_muted.name()));
  layout->addWidget(endpoint_);

  auto* ioa_row = new QHBoxLayout;
  auto* ioa_label = new QLabel{Tr("Information object address (IOA)")};
  ioa_label->setWordWrap(true);
  ioa_label->setStyleSheet(
      QStringLiteral("color:%1;").arg(tokens.fg_subtle.name()));
  ioa_edit_ = new QLineEdit;
  ioa_edit_->setObjectName(QStringLiteral("ioaEdit"));
  ioa_edit_->setValidator(new QIntValidator{0, 0x7fffffff, ioa_edit_});
  ioa_edit_->setFixedWidth(96);
  ioa_edit_->setStyleSheet(
      QStringLiteral(
          "QLineEdit{background:%1;color:%2;border:1px solid %3;"
          "border-radius:4px;padding:3px 6px;}")
          .arg(tokens.surface_muted.name(), tokens.fg.name(),
               tokens.border.name()));
  ioa_row->addWidget(ioa_label);
  ioa_row->addStretch(1);
  ioa_row->addWidget(ioa_edit_);
  layout->addLayout(ioa_row);

  layout->addStretch(1);

  // Revert / Apply bar.
  auto* actions = new QHBoxLayout;
  actions->addStretch(1);
  revert_ = new QPushButton{Tr("Revert")};
  revert_->setObjectName(QStringLiteral("revertRule"));
  apply_ = new QPushButton{Tr("Apply")};
  apply_->setObjectName(QStringLiteral("applyRule"));
  actions->addWidget(revert_);
  actions->addWidget(apply_);
  layout->addLayout(actions);

  connect(ioa_edit_, &QLineEdit::textEdited, this,
          [this](const QString&) { OnIoaEdited(); });
  connect(revert_, &QPushButton::clicked, this, [this] { OnRevert(); });
  connect(apply_, &QPushButton::clicked, this, [this] { OnApply(); });

  return view;
}

void TransmissionRuleInspector::Clear() {
  rule_id_ = scada::NodeId{};
  if (stack_)
    stack_->setCurrentIndex(0);
}

void TransmissionRuleInspector::ShowRule(const NodeRef& transmission) {
  if (!transmission ||
      !IsInstanceOf(transmission, scada::devices::id::TransmissionItemType)) {
    Clear();
    return;
  }

  const NodeRef source =
      transmission.target(scada::devices::id::HasTransmissionSource);
  const NodeRef endpoint = transmission.parent();
  const scada::Int32 ioa =
      transmission[scada::devices::id::TransmissionItemType_SourceAddress]
          .value()
          .get_or<scada::Int32>(0);

  const std::u16string source_name =
      source ? std::u16string(source.display_name()) : std::u16string{};

  TransmissionRuleDisplay rule;
  rule.node_id = transmission.node_id();
  rule.summary =
      QString::fromStdU16String(TransmissionRuleSummary(source_name, ioa));
  rule.protocol = QString::fromStdU16String(
      TransmissionProtocolLabel(transmission.type_definition().node_id()));
  rule.source_name = QString::fromStdU16String(source_name);
  rule.signal_tag = QString::fromStdU16String(
      source ? TransmissionSignalTag(source.type_definition().node_id())
             : std::u16string{});
  rule.endpoint = endpoint ? QString::fromStdU16String(endpoint.display_name())
                           : QString{};
  rule.ioa = ioa;

  ShowRuleDisplay(rule);
}

void TransmissionRuleInspector::ShowRuleDisplay(
    const TransmissionRuleDisplay& rule) {
  const scada::aui::ThemeTokens& tokens = PanelTokens();

  summary_->setText(rule.summary);
  protocol_->setText(rule.protocol);
  protocol_->setVisible(!rule.protocol.isEmpty());
  source_->setText(rule.source_name);
  endpoint_->setText(rule.endpoint);

  // The signal-type tag reads as a small accent pill; hidden when the source
  // has no simple TS/TI tag.
  signal_tag_->setText(rule.signal_tag);
  signal_tag_->setVisible(!rule.signal_tag.isEmpty());
  signal_tag_->setStyleSheet(
      QStringLiteral("#signalTagPill{color:%1;border:1px solid %1;"
                     "border-radius:8px;padding:0 8px;font-weight:600;}")
          .arg(tokens.accent.name()));

  // The IOA is editable only when the host supplied a write path (an
  // ApplyHandler). With none — e.g. the MainWindow shell, which has no
  // TaskManager — the inspector is read-only and the Revert/Apply bar is
  // hidden; editing rides the existing transmission grid's SourceAddress write.
  const bool editable = static_cast<bool>(apply_handler_);
  ioa_edit_->setReadOnly(!editable);
  revert_->setVisible(editable);
  apply_->setVisible(editable);

  rule_id_ = rule.node_id;
  live_ioa_ = rule.ioa;
  ioa_edit_->setText(QString::number(rule.ioa));
  UpdateDirtyState();

  stack_->setCurrentIndex(1);
}

bool TransmissionRuleInspector::dirty() const {
  if (!ioa_edit_)
    return false;
  bool ok = false;
  const int value = ioa_edit_->text().toInt(&ok);
  return ok && value != live_ioa_;
}

void TransmissionRuleInspector::OnIoaEdited() {
  UpdateDirtyState();
}

void TransmissionRuleInspector::UpdateDirtyState() {
  const bool is_dirty = dirty();
  revert_->setEnabled(is_dirty);
  apply_->setEnabled(is_dirty);
}

void TransmissionRuleInspector::OnRevert() {
  ioa_edit_->setText(QString::number(live_ioa_));
  UpdateDirtyState();
}

void TransmissionRuleInspector::OnApply() {
  bool ok = false;
  const int value = ioa_edit_->text().toInt(&ok);
  if (!ok)
    return;
  if (apply_handler_ && !rule_id_.is_null())
    apply_handler_(rule_id_, static_cast<scada::Int32>(value));
  // Reflect the applied value as the new live baseline; the model-change echo
  // will re-drive ShowRule, but updating now keeps the form non-dirty
  // immediately.
  live_ioa_ = static_cast<scada::Int32>(value);
  UpdateDirtyState();
}

TransmissionRuleInspector* MakeTransmissionRuleInspector() {
  if (scada::aui::GetSeverityTheme() == scada::aui::SeverityTheme::kLegacy)
    return nullptr;
  return new TransmissionRuleInspector;
}
