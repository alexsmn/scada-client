#pragma once

#include "base/boost_log.h"
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

  void DeleteUserName(std::u16string_view user_name);

  std::function<void(DataServices services)> completion_handler;
  std::function<void()> error_handler;

  std::vector<std::u16string> server_type_list;

  int server_type_index() const { return server_type_index_; }
  void SetServerTypeIndex(int index);

  std::u16string user_name;
  std::u16string password;
  std::string server_host;
  std::vector<std::u16string> user_list;
  // Automatic startup login is performed.
  bool auto_login = false;

 protected:
  void Connect(bool allow_remote_logoff);

  void OnLoginResult(const scada::Status& status);
  void OnLoginCompleted();
  void OnLoginFailed(const scada::Status& status);

  const std::shared_ptr<Executor> executor_;
  DataServicesContext services_context_;
  DialogService& dialog_service_;

  bool login_message_ = false;
  bool connecting_ = false;
  DataServices services_;

  std::string server_type_;
  int server_type_index_ = 0;

  struct ServerTypeData {
    std::string host;
  };

  std::vector<ServerTypeData> server_type_data_;

 private:
  BoostLogger logger_{LOG_NAME("LoginController")};
};
