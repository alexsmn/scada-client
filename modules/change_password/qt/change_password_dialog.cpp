#include "modules/change_password/change_password_dialog.h"

#include "aui/translation.h"
#include "modules/change_password/change_password.h"
#include "aui/dialog_service.h"
#include "ui_change_password_dialog.h"

#include <QMessageBox>

class ChangePasswordDialog : public QDialog, private ChangePasswordContext {
  Q_OBJECT

 public:
  explicit ChangePasswordDialog(ChangePasswordContext&& context,
                                QWidget* parent = nullptr);

 public Q_SLOTS:
  virtual void accept() override;

 private:
  Ui::ChangePasswordDialog ui;
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
    if (const char* violation = PasswordPolicyViolation(
            *policy_, new_password.toStdU16String())) {
      QMessageBox::critical(
          this, windowTitle(),
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
