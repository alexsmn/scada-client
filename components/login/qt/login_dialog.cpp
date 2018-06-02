#include "components/login/qt/login_dialog.h"

#include "core/session_service.h"
#include "core/status.h"

#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qdialogbuttonbox.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qlayout.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qmessagebox.h>
#include <QtWidgets/qpushbutton.h>
#include <QApplication>
#include <QCheckBox>
#include <QSettings>

namespace {

QStringList MakeQStringList(const std::vector<base::string16>& source) {
  QStringList list;
  list.reserve(source.size());
  for (auto& str : source)
    list.push_back(QString::fromStdWString(str));
  return list;
}

}  // namespace

LoginDialog::LoginDialog(DataServicesContext&& services_context)
    : controller_{std::move(services_context), dialog_service_} {
  ui.setupUi(this);

  dialog_service_.parent_widget = this;

  controller_.completion_handler = [this](DataServices& services) {
    this->services = std::move(services);
    QDialog::accept();
  };

  controller_.error_handler = [this] {
    EnableControls(true);
    ui.userNameComboBox->setFocus();
    ui.userNameComboBox->lineEdit()->selectAll();
  };

  ui.serverComboBox->setCurrentText(
      QString::fromStdString(controller_.server_host));

  ui.userNameComboBox->addItems(MakeQStringList(controller_.user_list));
  ui.userNameComboBox->setCurrentText(
      QString::fromStdWString(controller_.user_name));
  ui.userNameComboBox->lineEdit()->selectAll();

  ui.autoLoginCheckBox->setChecked(controller_.auto_login);

  if (controller_.auto_login) {
    ui.passwordLineEdit->setText(QString::fromStdWString(controller_.password));
    accept();
  }
}

LoginDialog::~LoginDialog() {}

void LoginDialog::accept() {
  EnableControls(false);

  controller_.server_host = ui.serverComboBox->currentText().toStdString();
  controller_.user_name = ui.userNameComboBox->currentText().toStdWString();
  controller_.password = ui.passwordLineEdit->text().toStdWString();
  controller_.auto_login = ui.autoLoginCheckBox->isChecked();

  controller_.Login();
}

void LoginDialog::EnableControls(bool enable) {
  ui.serverComboBox->setEnabled(enable);
  ui.userNameComboBox->setEnabled(enable);
  ui.passwordLineEdit->setEnabled(enable);
  ui.autoLoginCheckBox->setEnabled(enable);
  ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
}

bool ExecuteLoginDialog(DataServicesContext&& services_context,
                        DataServices& services) {
  LoginDialog login_dialog{std::move(services_context)};
  if (login_dialog.exec() == QDialog::Rejected)
    return false;
  services = std::move(login_dialog.services);
  return true;
}
