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
  // TODO: Reg key constants.
  QSettings settings("HKEY_CURRENT_USER\\Software\\Telecontrol\\Workplace", QSettings::NativeFormat);
  auto user_name = settings.value("User").toString();
  user_list_ = settings.value("UserList").toString().split(',');
  auto host = settings.value("Host").toString();
  auto password = settings.value("Password").toString();
  auto auto_login = settings.value("AutoLogin").toBool();
  if (QApplication::queryKeyboardModifiers() & Qt::ControlModifier)
    auto_login = false;

  setWindowTitle(tr("Login"));

  button_box_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(button_box_, &QDialogButtonBox::accepted, [this] { StartLogin(); });
  connect(button_box_, &QDialogButtonBox::rejected, [this] { reject(); });

  name_combo_box_ = new QComboBox;
  name_combo_box_->setEditable(true);
  name_combo_box_->setMinimumSize(QSize(200, 0));
  name_combo_box_->addItems(user_list_);
  name_combo_box_->setCurrentText(user_name);
  name_combo_box_->lineEdit()->selectAll();

  auto* name_label = new QLabel(tr("&Name:"));
  name_label->setBuddy(name_combo_box_);

  password_edit_ = new QLineEdit;
  password_edit_->setEchoMode(QLineEdit::Password);
  if (auto_login)
    password_edit_->setText(password);

  auto* password_label = new QLabel(tr("&Password:"));
  password_label->setBuddy(password_edit_);

  auto_login_check_box_ = new QCheckBox(tr("&Auto-Login"));
  auto_login_check_box_->setChecked(auto_login);

  auto* layout = new QGridLayout;
  const int kMargin = 15;
  layout->setMargin(kMargin);
  layout->setSpacing(kMargin);
  layout->addWidget(name_label, 0, 0, Qt::AlignRight);
  layout->addWidget(name_combo_box_, 0, 1);
  layout->addWidget(password_label, 1, 0, Qt::AlignRight);
  layout->addWidget(password_edit_, 1, 1);
  layout->addWidget(auto_login_check_box_, 2, 1);
  layout->addWidget(button_box_, 3, 0, 1, 2);

  setLayout(layout);

  if (auto_login)
    StartLogin();
}

LoginDialog::~LoginDialog() {
}

void LoginDialog::StartLogin() {
  SetControlsEnabled(false);

  user_name_ = name_combo_box_->currentText();
  password_ = password_edit_->text();
  auto_login_ = auto_login_check_box_->isChecked();

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
    name_combo_box_->setFocus();
    name_combo_box_->lineEdit()->selectAll();
  }
}

void LoginDialog::SetControlsEnabled(bool enabled) {
  name_combo_box_->setEnabled(enabled);
  password_edit_->setEnabled(enabled);
  auto_login_check_box_->setEnabled(enabled);
  button_box_->button(QDialogButtonBox::Ok)->setEnabled(enabled);
}
