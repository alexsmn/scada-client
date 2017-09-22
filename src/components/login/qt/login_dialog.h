#pragma once

#include <memory>
#include <QDialog>

#include "ui_login_dialog.h"
#include "core/status.h"
#include "services/data_services.h"
#include "services/data_services_factory.h"

namespace scada {
class SessionService;
}

class QDialogButtonBox;
class QCheckBox;
class QComboBox;
class QLineEdit;

class LoginDialog : public QDialog {
	Q_OBJECT

 public:
  explicit LoginDialog(const DataServicesContext& services_context);
  ~LoginDialog();

  DataServices& services() { return services_; }

public Q_SLOTS:
    virtual void accept() override;

 private:
  void OnLoginResult(const scada::Status& result);

  void SetControlsEnabled(bool enabled);

  DataServicesContext services_context_;
  DataServices services_;

	Ui::LoginDialog ui;

  QString user_name_;
  QString password_;
  QString server_;
  bool auto_login_ = false;
  QStringList user_list_;
  
  std::shared_ptr<bool> cancelation_;
};