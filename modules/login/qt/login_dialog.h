#pragma once

#include "base/any_executor.h"

#include "aui/qt/dialog_service_impl_qt.h"
#include "base/async_completion.h"
#include "modules/login/login_dialog.h"
#include "scada/data_services_factory.h"
#include "ui_login_dialog.h"

#include <QDialog>
#include <memory>

class LoginController;
class QDialogButtonBox;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;

class LoginDialog : public QDialog {
  Q_OBJECT

 public:
  // `settings_store` — see ExecuteLoginDialog in modules/login/login_dialog.h.
  LoginDialog(AnyExecutor executor,
              DataServicesContext&& services_context,
              std::shared_ptr<SettingsStore> settings_store = {});
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

  // Shows the OPC UA security fields only when the selected backend supports
  // them (see LoginController::IsSecuritySupported).
  void UpdateSecurityVisibility();

  // Opens a file picker and writes the chosen path into `target`.
  void BrowseForFile(QLineEdit& target, const QString& title);

  // Wraps the .ui form in the workbench chrome (brand lockup + connection
  // summary), leaving the form itself untouched.
  void BuildReshellChrome();
  // Re-reads the backend/server fields into the connection summary line.
  void RefreshConnectionSummary();

  Ui::LoginDialog ui;

  // The "connecting to" line; null until BuildReshellChrome has run.
  QLabel* connection_summary_ = nullptr;

  DialogServiceImplQt dialog_service_;

  const std::shared_ptr<LoginController> controller_;
  scada::base::AsyncCompletion completion_;
  std::optional<DataServices> result_;
  bool completed_ = false;
};
