#include "components/login/qt/login_dialog.h"

#include "base/e2e_test_hooks.h"
#include "components/login/login_controller.h"
#include "scada/session_service.h"
#include "scada/status.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QKeyEvent>
#include <QSettings>
#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qdialogbuttonbox.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qlayout.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qmessagebox.h>
#include <QtWidgets/qpushbutton.h>

namespace {

QStringList MakeQStringList(const std::vector<std::u16string>& source) {
  QStringList list;
  list.reserve(source.size());
  for (auto& str : source)
    list.push_back(QString::fromStdU16String(str));
  return list;
}

}  // namespace

LoginDialog::LoginDialog(std::shared_ptr<Executor> executor,
                         DataServicesContext&& services_context)
    : controller_{std::make_shared<LoginController>(
          std::move(executor),
          std::move(services_context),
          dialog_service_,
          client::CreateE2eSettingsStore())} {
  ui.setupUi(this);

  dialog_service_.parent_widget = this;

  controller_->completion_handler = [this](DataServices services) {
    promise.resolve(std::move(services));
  };

  controller_->login_failed_handler = [this](const scada::Status& status) {
    if (!client::IsE2eTestMode())
      return false;

    client::ReportE2eStatus(
        std::string{"failure: "} + ToString(status));
    promise.resolve(std::nullopt);
    close();
    return true;
  };

  controller_->error_handler = [this] {
    EnableControls(true);
    ui.userNameComboBox->setFocus();
    ui.userNameComboBox->lineEdit()->selectAll();
  };

  ui.serverTypeComboBox->addItems(
      MakeQStringList(controller_->server_type_list));
  ui.serverTypeComboBox->setCurrentIndex(controller_->server_type_index());
  ui.serverTypeComboBox->setVisible(controller_->server_type_list.size() >= 2);
  connect(ui.serverTypeComboBox,
          qOverload<int>(&QComboBox::currentIndexChanged), this,
          [this](int server_type_index) {
            controller_->server_host =
                ui.serverComboBox->currentText().toStdString();
            controller_->SetServerTypeIndex(server_type_index);
            ui.serverComboBox->setCurrentText(
                QString::fromStdString(controller_->server_host));
          });

  ui.serverComboBox->setCurrentText(
      QString::fromStdString(controller_->server_host));

  ui.userNameComboBox->addItems(MakeQStringList(controller_->user_list));
  ui.userNameComboBox->setCurrentText(
      QString::fromStdU16String(controller_->user_name));
  ui.userNameComboBox->lineEdit()->selectAll();

  ui.autoLoginCheckBox->setChecked(controller_->auto_login);

  ui.userNameComboBox->view()->setToolTip(
      tr("You can remove the highlighted user from list by pressing Delete."));

  QApplication::instance()->installEventFilter(this);

  if (controller_->auto_login) {
    ui.passwordLineEdit->setText(
        QString::fromStdU16String(controller_->password));
    Login();
  }
}

LoginDialog::~LoginDialog() {
  QApplication::instance()->removeEventFilter(this);
}

void LoginDialog::accept() {
  Login();
}

void LoginDialog::reject() {
  if (client::IsE2eTestMode()) {
    client::ReportE2eStatusIfUnset("canceled");
  }
  promise.resolve(std::nullopt);
}

void LoginDialog::Login() {
  EnableControls(false);

  controller_->SetServerTypeIndex(controller_->server_type_list.size() >= 2
                                      ? ui.serverTypeComboBox->currentIndex()
                                      : 0);
  controller_->server_host = ui.serverComboBox->currentText().toStdString();
  controller_->user_name = ui.userNameComboBox->currentText().toStdU16String();
  controller_->password = ui.passwordLineEdit->text().toStdU16String();
  controller_->auto_login = ui.autoLoginCheckBox->isChecked();

  controller_->Login();
}

void LoginDialog::EnableControls(bool enable) {
  ui.serverTypeComboBox->setEnabled(enable);
  ui.serverComboBox->setEnabled(enable);
  ui.userNameComboBox->setEnabled(enable);
  ui.passwordLineEdit->setEnabled(enable);
  ui.autoLoginCheckBox->setEnabled(enable);
  ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
}

bool LoginDialog::eventFilter(QObject* object, QEvent* event) {
  if (object == ui.userNameComboBox->view()) {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
      if (key_event->key() == Qt::Key_Delete) {
        int index = ui.userNameComboBox->view()->currentIndex().row();
        if (index != -1) {
          controller_->DeleteUserName(
              ui.userNameComboBox->itemText(index).toStdU16String());
          ui.userNameComboBox->removeItem(index);
        }
        return true;
      }
    }
  }

  return QDialog::eventFilter(object, event);
}

promise<std::optional<DataServices>> ExecuteLoginDialog(
    std::shared_ptr<Executor> executor,
    DataServicesContext&& services_context) {
  LoginDialog* login_dialog =
      new LoginDialog{std::move(executor), std::move(services_context)};

  auto promise = login_dialog->promise.then(
      [login_dialog](const std::optional<DataServices>& services) {
        login_dialog->deleteLater();
        return services;
      });

  login_dialog->setModal(true);
  login_dialog->show();
  return promise;
}
