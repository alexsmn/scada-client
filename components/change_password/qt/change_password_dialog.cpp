#include "components/change_password/change_password_dialog.h"

#include "components/change_password/change_password.h"
#include "services/dialog_service.h"
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

  ChangePassword(*this, current_password.toStdU16String(),
                 new_password.toStdU16String());

  QDialog::accept();
}

void ShowChangePasswordDialog(DialogService& dialog_service,
                              ChangePasswordContext&& context) {
  ChangePasswordDialog dialog{std::move(context),
                              dialog_service.GetParentWidget()};
  dialog.exec();
}
