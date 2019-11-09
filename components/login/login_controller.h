#pragma once

#include "base/memory/weak_ptr.h"
#include "core/data_services_factory.h"
#include "core/localized_text.h"

namespace scada {
class Status;
}

class DialogService;

class LoginController {
 public:
  LoginController(DataServicesContext&& services_context,
                  DialogService& dialog_service);

  void Login();

  void DeleteUserName(base::StringPiece16 user_name);

  std::function<void(DataServices& services)> completion_handler;
  std::function<void()> error_handler;

  std::vector<base::string16> server_type_list;
  int server_type_index = 0;

  base::string16 user_name;
  base::string16 password;
  std::string server_host;
  std::vector<base::string16> user_list;
  // Automatic startup login is performed.
  bool auto_login = false;

 protected:
  void Connect(bool allow_remote_logoff);

  void OnLoginResult(const scada::Status& status);
  void OnLoginCompleted();
  void OnLoginFailed(const scada::Status& status);

  base::string16 GetUserListString() const;

  DataServicesContext services_context_;
  DialogService& dialog_service_;

  bool login_message_ = false;
  bool connecting_ = false;
  DataServices services_;

  std::string server_type_;

  base::WeakPtrFactory<LoginController> weak_factory_{this};
};
