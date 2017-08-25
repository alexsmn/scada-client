#pragma once

#include <memory>
#include <QtWidgets/qdialog.h>

#include "core/status.h"

namespace scada {
class SessionService;
}

class QDialogButtonBox;
class QCheckBox;
class QComboBox;
class QLineEdit;

class LoginDialog : public QDialog {
 public:
  explicit LoginDialog(scada::SessionService& session);
  ~LoginDialog();

 private:
  void StartLogin();

  void OnCreateSessionResult(const scada::Status& result);

  void SetControlsEnabled(bool enabled);

  scada::SessionService& session_;

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