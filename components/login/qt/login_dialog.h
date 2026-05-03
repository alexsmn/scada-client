#pragma once

#include "base/any_executor.h"

#include "base/async_completion.h"
#include "components/login/login_dialog.h"
#include "scada/data_services_factory.h"
#include "aui/qt/dialog_service_impl_qt.h"
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
  LoginDialog(AnyExecutor executor,
              DataServicesContext&& services_context);
  ~LoginDialog();

  Awaitable<std::optional<DataServices>> Wait();

 protected:
  virtual bool eventFilter(QObject* object, QEvent* event) override;
  virtual void accept() override;
  virtual void reject() override;

 private:
  void Login();
  void Complete(std::optional<DataServices> services);

  void EnableControls(bool enable);

  Ui::LoginDialog ui;

  DialogServiceImplQt dialog_service_;

  const std::shared_ptr<LoginController> controller_;
  base::AsyncCompletion completion_;
  std::optional<DataServices> result_;
  bool completed_ = false;
};
