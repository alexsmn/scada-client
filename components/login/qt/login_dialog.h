#pragma once

#include "components/login/login_dialog.h"
#include "scada/data_services_factory.h"
#include "qt/dialog_service_impl_qt.h"
#include "ui_login_dialog.h"

#include <QDialog>
#include <memory>

class LoginController;
class QDialogButtonBox;
class QCheckBox;
class QComboBox;
class QLineEdit;

class LoginDialog : public QDialog {
  Q_OBJECT

 public:
  LoginDialog(std::shared_ptr<Executor> executor,
              DataServicesContext&& services_context);
  ~LoginDialog();

  promise<std::optional<DataServices>> promise;

 protected:
  virtual bool eventFilter(QObject* object, QEvent* event) override;
  virtual void accept() override;
  virtual void reject() override;

 private:
  void Login();

  void EnableControls(bool enable);

  Ui::LoginDialog ui;

  DialogServiceImplQt dialog_service_;

  const std::shared_ptr<LoginController> controller_;
};
