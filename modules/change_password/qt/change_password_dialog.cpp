#include "modules/change_password/change_password_dialog.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "modules/change_password/change_password.h"
#include "ui_change_password_dialog.h"
#include "user_access/password_policy.h"

#include <QLabel>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>

#include <optional>
#include <string_view>
#include <vector>

namespace {

QString Tr(std::string_view text) {
  return QString::fromStdU16String(Translate(text));
}

// The length rule as prose, or empty when the server imposes none. A
// non-positive bound is "unconstrained" — what a server with no length policy
// publishes — never a zero-length rule.
QString LengthText(const PasswordPolicy& policy) {
  const bool has_min = policy.min_length > 0;
  const bool has_max = policy.max_length > 0;
  if (!has_min && !has_max) {
    return {};
  }
  QString range;
  if (has_min && has_max) {
    range = QStringLiteral("%1–%2")
                .arg(static_cast<int>(policy.min_length))
                .arg(static_cast<int>(policy.max_length));
  } else if (has_min) {
    range = QStringLiteral("≥ %1").arg(static_cast<int>(policy.min_length));
  } else {
    range = QStringLiteral("≤ %1").arg(static_cast<int>(policy.max_length));
  }
  return QStringLiteral("%1: %2").arg(Tr("Length"), range);
}

}  // namespace

class ChangePasswordDialog : public QDialog, private ChangePasswordContext {
  Q_OBJECT

 public:
  explicit ChangePasswordDialog(ChangePasswordContext&& context,
                                QWidget* parent = nullptr);

  // Takes the policy the server published (nullopt: it could not be read).
  void SetPolicy(std::optional<PasswordPolicy> policy);

 public Q_SLOTS:
  virtual void accept() override;

 private:
  // One rule as the operator reads it, and the policy that judges it alone —
  // nullopt for the server's own wording, which the client cannot check.
  struct Rule {
    QLabel* label = nullptr;
    QString text;
    std::optional<PasswordPolicy> judge;
  };

  // Starts the policy read; the dialog may be closed before it completes.
  void ReadPolicy();

  // Re-ticks every rule against the new password and enables OK only when
  // the password satisfies the whole policy.
  void UpdateRules();

  Ui::ChangePasswordDialog ui;

  // The policy the new password will actually be validated against, read from
  // the server (Part 18 §5.2.2). Nullopt until read, and when it could not be
  // read, in which case the client imposes nothing and lets the server judge.
  std::optional<PasswordPolicy> policy_;

  // Holds the rule lines; hidden while there is no policy, because an
  // unreadable policy must not look like "no rules" (dialogs.html).
  QWidget* rules_box_ = nullptr;
  std::vector<Rule> rules_;
};

#include "change_password_dialog.moc"

ChangePasswordDialog::ChangePasswordDialog(ChangePasswordContext&& context,
                                           QWidget* parent)
    : QDialog{parent}, ChangePasswordContext{std::move(context)} {
  ui.setupUi(this);

  // An administrator resetting someone else's account does not supply the old
  // password (OPC UA Part 18 §5.2.7), so the field is hidden rather than
  // collected and discarded — asking for a credential that is neither checked
  // nor sent teaches the operator the wrong thing about what is happening.
  if (!self_service_) {
    ui.currentLabel->setVisible(false);
    ui.currentLineEdit->setVisible(false);
  }

  rules_box_ = new QWidget{this};
  rules_box_->setObjectName(QStringLiteral("changePasswordRules"));
  new QVBoxLayout{rules_box_};
  rules_box_->layout()->setContentsMargins(0, 0, 0, 0);
  rules_box_->setVisible(false);
  ui.verticalLayout->insertWidget(1, rules_box_);

  connect(ui.newLineEdit, &QLineEdit::textChanged, this,
          &ChangePasswordDialog::UpdateRules);

  ReadPolicy();
}

void ChangePasswordDialog::ReadPolicy() {
  if (!node_service_) {
    return;
  }
  // The read outlives nothing it does not own: the dialog is reached only
  // through a QPointer, so closing the form before the server answers drops
  // the answer instead of touching a deleted widget.
  CoSpawn(executor_,
          [executor = executor_, &node_service = *node_service_,
           self = QPointer<ChangePasswordDialog>{
               this}]() mutable -> Awaitable<void> {
            std::optional<PasswordPolicy> policy =
                co_await ReadPasswordPolicy(executor, node_service);
            if (self) {
              self->SetPolicy(std::move(policy));
            }
          });
}

void ChangePasswordDialog::SetPolicy(std::optional<PasswordPolicy> policy) {
  policy_ = std::move(policy);

  for (Rule& rule : rules_) {
    delete rule.label;
  }
  rules_.clear();

  if (policy_) {
    if (QString length = LengthText(*policy_); !length.isEmpty()) {
      rules_.push_back(
          {.text = length,
           .judge = PasswordPolicy{.min_length = policy_->min_length,
                                   .max_length = policy_->max_length}});
    }
    for (const PasswordRequirement& requirement :
         PasswordRequirementsFor(policy_->options)) {
      if (requirement.required) {
        rules_.push_back(
            {.text = Tr(requirement.label),
             .judge = PasswordPolicy{.options = requirement.option}});
      }
    }
    // The server's own wording, verbatim and unticked: it may state a rule
    // the client cannot check, and paraphrasing it would put two
    // descriptions of one rule out of step.
    if (!policy_->restrictions.empty()) {
      rules_.push_back(
          {.text = QString::fromStdU16String(policy_->restrictions)});
    }
    for (Rule& rule : rules_) {
      rule.label = new QLabel{rules_box_};
      rule.label->setWordWrap(true);
      rules_box_->layout()->addWidget(rule.label);
    }
  }

  rules_box_->setVisible(!rules_.empty());
  UpdateRules();
}

void ChangePasswordDialog::UpdateRules() {
  const std::u16string password = ui.newLineEdit->text().toStdU16String();
  for (Rule& rule : rules_) {
    if (!rule.judge) {
      rule.label->setText(rule.text);
      continue;
    }
    const bool met = PasswordPolicyViolation(*rule.judge, password) == nullptr;
    // A mark and the words, never colour alone.
    rule.label->setText(QStringLiteral("%1 %2").arg(
        met ? QStringLiteral("✓") : QStringLiteral("·"), rule.text));
  }

  if (QPushButton* ok = ui.buttonBox->button(QDialogButtonBox::Ok)) {
    ok->setEnabled(!policy_ ||
                   PasswordPolicyViolation(*policy_, password) == nullptr);
  }
}

void ChangePasswordDialog::accept() {
  auto current_password = ui.currentLineEdit->text();
  auto new_password = ui.newLineEdit->text();
  auto repeat_password = ui.repeatLineEdit->text();

  if (new_password != repeat_password) {
    QMessageBox::critical(this, windowTitle(),
                          tr("New and repeated password do not match."));
    return;
  }

  // A courtesy check against the policy the SERVER published, never the
  // enforcement: the server validates every password itself, and a client that
  // trusted its own verdict would let a deployment-specific rule through. It
  // is here so the operator learns of a problem before the round trip. With no
  // policy read, nothing is imposed.
  if (policy_) {
    if (const char* violation =
            PasswordPolicyViolation(*policy_, new_password.toStdU16String())) {
      QMessageBox::critical(this, windowTitle(),
                            QString::fromStdU16String(Translate(violation)));
      return;
    }
  }

  ChangePassword(*this, current_password.toStdU16String(),
                 new_password.toStdU16String());

  QDialog::accept();
}

void ShowChangePasswordDialog(DialogService& dialog_service,
                              ChangePasswordContext&& context) {
  ChangePasswordDialog* dialog = new ChangePasswordDialog{
      std::move(context), dialog_service.GetParentWidget()};
  dialog->setModal(false);
  QObject::connect(dialog, &QDialog::finished, dialog, &QObject::deleteLater);
  dialog->show();
}
