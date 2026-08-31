#include "user_access/qt/password_policy_panel.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include <string_view>

namespace {

const scada::aui::ThemeTokens& PanelTokens() {
  return scada::aui::ActiveThemeTokens();
}

QString Tr(std::string_view text) {
  return QString::fromStdU16String(Translate(text));
}

// The length range as prose. A non-positive bound is "unconstrained", which is
// what a server with no length policy publishes — not a zero-length rule.
QString LengthText(const PasswordPolicy& policy) {
  const bool has_min = policy.min_length > 0;
  const bool has_max = policy.max_length > 0;
  if (!has_min && !has_max) {
    return Tr("Any length");
  }
  if (has_min && has_max) {
    return QStringLiteral("%1–%2")
        .arg(static_cast<int>(policy.min_length))
        .arg(static_cast<int>(policy.max_length));
  }
  return has_min
             ? QStringLiteral("≥ %1").arg(static_cast<int>(policy.min_length))
             : QStringLiteral("≤ %1").arg(static_cast<int>(policy.max_length));
}

}  // namespace

PasswordPolicyPanel::PasswordPolicyPanel(QWidget* parent) : QWidget{parent} {
  const scada::aui::ThemeTokens& tokens = PanelTokens();
  setObjectName(QStringLiteral("passwordPolicyPanel"));
  setStyleSheet(QStringLiteral("#passwordPolicyPanel{background:%1;}")
                    .arg(tokens.bg.name()));

  auto* root = new QVBoxLayout{this};
  root->setContentsMargins(12, 10, 12, 12);
  root->setSpacing(10);
  root->addWidget(BuildHeader());

  auto* length_row = new QHBoxLayout;
  auto* length_label = new QLabel{Tr("Length")};
  length_label->setStyleSheet(
      QStringLiteral("color:%1;").arg(tokens.fg_subtle.name()));
  length_ = new QLabel;
  length_->setObjectName(QStringLiteral("passwordPolicyLength"));
  length_->setStyleSheet(QStringLiteral("color:%1;").arg(tokens.fg.name()));
  length_row->addWidget(length_label);
  length_row->addStretch(1);
  length_row->addWidget(length_);
  root->addLayout(length_row);

  auto* requirements_header = new QLabel{Tr("Must contain")};
  requirements_header->setStyleSheet(
      QStringLiteral("color:%1;font-size:10px;font-weight:600;"
                     "text-transform:uppercase;letter-spacing:.5px;")
          .arg(tokens.fg_subtle.name()));
  root->addWidget(requirements_header);

  auto* requirements_host = new QWidget;
  requirements_ = new QVBoxLayout{requirements_host};
  requirements_->setContentsMargins(0, 0, 0, 0);
  requirements_->setSpacing(2);
  root->addWidget(requirements_host);

  restrictions_ = new QLabel;
  restrictions_->setObjectName(QStringLiteral("passwordPolicyRestrictions"));
  restrictions_->setWordWrap(true);
  restrictions_->setStyleSheet(
      QStringLiteral("color:%1;").arg(tokens.fg_muted.name()));
  root->addWidget(restrictions_);
  root->addStretch(1);
}

PasswordPolicyPanel::~PasswordPolicyPanel() = default;

QWidget* PasswordPolicyPanel::BuildHeader() {
  const scada::aui::ThemeTokens& tokens = PanelTokens();
  title_ = new QLabel{Tr("Password policy")};
  title_->setObjectName(QStringLiteral("passwordPolicyTitle"));
  title_->setStyleSheet(
      QStringLiteral("color:%1;font-size:14px;font-weight:600;")
          .arg(tokens.fg.name()));
  return title_;
}

void PasswordPolicyPanel::ShowPolicy(
    const std::optional<PasswordPolicy>& policy) {
  const scada::aui::ThemeTokens& tokens = PanelTokens();

  while (QLayoutItem* item = requirements_->takeAt(0)) {
    if (QWidget* widget = item->widget())
      widget->deleteLater();
    delete item;
  }

  // An unreadable policy says nothing about the rules, so no requirement row
  // is drawn at all — listing them all as "not required" would be as false a
  // claim as listing them as required.
  if (!policy) {
    length_->setText(Tr("No data"));
    length_->setStyleSheet(
        QStringLiteral("color:%1;").arg(tokens.fg_subtle.name()));
    restrictions_->setText(Tr("The password policy could not be read."));
    auto* placeholder = new QLabel{QStringLiteral("—")};
    placeholder->setObjectName(
        QStringLiteral("passwordPolicyRequirementsPlaceholder"));
    placeholder->setStyleSheet(
        QStringLiteral("color:%1;").arg(tokens.fg_subtle.name()));
    requirements_->addWidget(placeholder);
    return;
  }

  length_->setText(LengthText(*policy));
  length_->setStyleSheet(QStringLiteral("color:%1;").arg(tokens.fg.name()));

  for (const PasswordRequirement& requirement :
       PasswordRequirementsFor(policy->options)) {
    auto* row = new QWidget;
    auto* row_layout = new QHBoxLayout{row};
    row_layout->setContentsMargins(0, 2, 0, 2);
    row_layout->setSpacing(8);

    auto* mark = new QLabel;
    mark->setFixedSize(14, 14);
    mark->setStyleSheet(
        requirement.required
            ? QStringLiteral("background:%1;border-radius:4px;")
                  .arg(tokens.accent.name())
            : QStringLiteral("border:1px solid %1;border-radius:4px;")
                  .arg(tokens.border_strong.name()));

    auto* label = new QLabel{Tr(requirement.label)};
    label->setStyleSheet(
        QStringLiteral("color:%1;")
            .arg((requirement.required ? tokens.fg : tokens.fg_subtle).name()));

    row_layout->addWidget(mark);
    row_layout->addWidget(label);
    row_layout->addStretch(1);
    requirements_->addWidget(row);
  }

  // The server's own wording, verbatim: paraphrasing it here would put two
  // descriptions of one rule out of step.
  restrictions_->setText(QString::fromStdU16String(policy->restrictions));
}

PasswordPolicyPanel* MakePasswordPolicyPanel() {
  return new PasswordPolicyPanel;
}
