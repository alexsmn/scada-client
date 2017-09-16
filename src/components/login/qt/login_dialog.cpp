#include "components/login/qt/login_dialog.h"

#include <QtCore/qsettings.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qcheckbox.h>
#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qdialogbuttonbox.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qlayout.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qmessagebox.h>
#include <QtWidgets/qpushbutton.h>

#include "core/status.h"
#include "core/session_service.h"

LoginDialog::LoginDialog(const DataServicesContext& services_context)
    : services_context_(services_context),
      cancelation_(std::make_shared<bool>(false)) {
	ui.setupUi(this);

  // TODO: Reg key constants.
  QSettings settings("HKEY_CURRENT_USER\\Software\\Telecontrol\\Workplace", QSettings::NativeFormat);
  auto user_name = settings.value("User").toString();
  user_list_ = settings.value("UserList").toString().split(',');
  auto host = settings.value("Host").toString();
  auto password = settings.value("Password").toString();
  auto auto_login = settings.value("AutoLogin").toBool();
  if (QApplication::queryKeyboardModifiers() & Qt::ControlModifier)
    auto_login = false;

  connect(ui.buttonBox, &QDialogButtonBox::accepted, [this] { StartLogin(); });
  connect(ui.buttonBox, &QDialogButtonBox::rejected, [this] { reject(); });

  ui.serverComboBox->setCurrentText(host);

  ui.userNameComboBox->addItems(user_list_);
  ui.userNameComboBox->setCurrentText(user_name);
  ui.userNameComboBox->lineEdit()->selectAll();

  if (auto_login)
    ui.passwordLineEdit->setText(password);

  ui.autoLoginCheckBox->setChecked(auto_login);

  if (auto_login)
    StartLogin();
}

LoginDialog::~LoginDialog() {
}

void LoginDialog::StartLogin() {
  SetControlsEnabled(false);

  user_name_ = ui.userNameComboBox->currentText();
  password_ = ui.passwordLineEdit->text();
  auto_login_ = ui.autoLoginCheckBox->isChecked();

  if (!CreateDataServices("Scada", services_context_, services_)) {
    OnLoginResult(scada::StatusCode::Bad_UnsupportedProtocolVersion);
    return;
  }

  std::weak_ptr<bool> cancelation = cancelation_;
  services_.session_service_->Connect("TCP;active;port=2000", user_name_.toStdString(), password_.toStdString(), true,
      [this, cancelation](const scada::Status& result) {
        if (!cancelation.expired())
          OnLoginResult(result);
      });
}

void LoginDialog::OnLoginResult(const scada::Status& result) {
  if (result) {
    QSettings settings("HKEY_CURRENT_USER\\Software\\Telecontrol\\Workplace", QSettings::NativeFormat);
    settings.setValue("User", user_name_);
    settings.setValue("Password", auto_login_ ? password_ : QString());
    settings.setValue("AutoLogin", auto_login_);

    int i = user_list_.indexOf(user_name_);
    if (!user_list_.empty() && i != user_list_.size() - 1) {
      if (i != -1)
        user_list_.erase(user_list_.begin() + i);
      user_list_.append(user_name_);
      const int sc_maxUserList = 10;
      if (user_list_.size() > sc_maxUserList)
        user_list_.erase(user_list_.begin(), user_list_.begin() + user_list_.size() - sc_maxUserList);
      settings.setValue("UserList", user_list_.join(','));
    }

    accept();

  } else {
    auto_login_ = false;

    QMessageBox msg_box;
    msg_box.setWindowTitle(tr("Login failed"));
    msg_box.setIcon(QMessageBox::Icon::Warning);
    msg_box.setText(QString::fromStdWString(result.ToString16()) + ".");
    msg_box.exec();

    SetControlsEnabled(true);
    ui.userNameComboBox->setFocus();
    ui.userNameComboBox->lineEdit()->selectAll();
  }
}

void LoginDialog::SetControlsEnabled(bool enabled) {
  ui.userNameComboBox->setEnabled(enabled);
  ui.passwordLineEdit->setEnabled(enabled);
  ui.autoLoginCheckBox->setEnabled(enabled);
  ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enabled);
}
