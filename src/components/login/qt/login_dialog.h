#pragma once

#include <memory>
#include <QtWidgets/qdialog.h>

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
 public:
  explicit LoginDialog(const DataServicesContext& services_context);
  ~LoginDialog();

  DataServices& services() { return services_; }

 private:
  void StartLogin();

  void OnLoginResult(const scada::Status& result);

  void SetControlsEnabled(bool enabled);

  DataServicesContext services_context_;
  DataServices services_;

  QComboBox* name_combo_box_ = nullptr;
  QLineEdit* password_edit_ = nullptr;
  QCheckBox* auto_login_check_box_ = nullptr;
  QDialogButtonBox* button_box_ = nullptr;

  QString user_name_;
  QString password_;
  QString host_;
  bool auto_login_ = false;
  QStringList user_list_;
  
  std::shared_ptr<bool> cancelation_;
};