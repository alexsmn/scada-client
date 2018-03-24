#pragma once

#include "components/login/login_controller.h"
#include "core/data_services_factory.h"
#include "qt/dialog_service_impl_qt.h"
#include "ui_login_dialog.h"

#include <QDialog>
#include <memory>

class QDialogButtonBox;
class QCheckBox;
class QComboBox;
class QLineEdit;

class LoginDialog : public QDialog {
  Q_OBJECT

 public:
  explicit LoginDialog(DataServicesContext&& services_context);
  ~LoginDialog();

  DataServices services;

 public Q_SLOTS:
  virtual void accept() override;

 private:
  void Login();
  void EnableControls(bool enable);

  Ui::LoginDialog ui;

  DialogServiceImplQt dialog_service_;
  LoginController controller_;
};
