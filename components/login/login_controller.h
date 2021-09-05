#pragma once

#include "core/data_services_factory.h"
#include "core/localized_text.h"

#include <memory>

namespace scada {
class Status;
}

class Executor;
class DialogService;

class LoginController : public std::enable_shared_from_this<LoginController> {
 public:
  LoginController(std::shared_ptr<Executor> executor,
                  DataServicesContext&& services_context,
                  DialogService& dialog_service);

  void Login();

  void DeleteUserName(std::wstring_view user_name);

  std::function<void(DataServices services)> completion_handler;
  std::function<void()> error_handler;

  std::vector<std::wstring> server_type_list;
  int server_type_index = 0;

  std::wstring user_name;
  std::wstring password;
  std::string server_host;
  std::vector<std::wstring> user_list;
  // Automatic startup login is performed.
  bool auto_login = false;

 protected:
  void Connect(bool allow_remote_logoff);

  void OnLoginResult(const scada::Status& status);
  void OnLoginCompleted();
  void OnLoginFailed(const scada::Status& status);

  std::wstring GetUserListString() const;

  const std::shared_ptr<Executor> executor_;
  DataServicesContext services_context_;
  DialogService& dialog_service_;

  bool login_message_ = false;
  bool connecting_ = false;
  DataServices services_;

  std::string server_type_;
};
