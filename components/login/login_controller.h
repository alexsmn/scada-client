#pragma once

#include "aui/dialog_service.h"
#include "base/awaitable.h"
#include "base/settings_store.h"
#include "base/boost_log.h"
#include "scada/data_services_factory.h"
#include "scada/localized_text.h"

#include <memory>

namespace scada {
struct SessionConnectParams;
class SessionService;
class Status;
}

class Executor;
class DialogService;

class LoginController : public std::enable_shared_from_this<LoginController> {
 public:
  LoginController(std::shared_ptr<Executor> executor,
                  DataServicesContext&& services_context,
                  DialogService& dialog_service,
                  std::shared_ptr<SettingsStore> settings_store = {});

  void Login();

  void DeleteUserName(std::u16string_view user_name);

  std::function<void(DataServices services)> completion_handler;
  std::function<void()> error_handler;
  std::function<bool(const scada::Status& status)> login_failed_handler;

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

  static Awaitable<void> ConnectAsync(std::shared_ptr<Executor> executor,
                                      std::weak_ptr<LoginController> controller,
                                      scada::SessionService& session_service,
                                      scada::SessionConnectParams params);
  static Awaitable<void> CompleteLoginAsync(
      std::shared_ptr<Executor> executor,
      std::function<void(DataServices services)> completion_handler,
      DataServices services,
      promise<void> message);
  static Awaitable<void> PromptForceLogoffAsync(
      std::shared_ptr<Executor> executor,
      std::weak_ptr<LoginController> controller,
      promise<MessageBoxResult> prompt);
  static Awaitable<void> ReportLoginErrorAsync(
      std::shared_ptr<Executor> executor,
      std::weak_ptr<LoginController> controller,
      promise<MessageBoxResult> prompt);

  const std::shared_ptr<Executor> executor_;
  DataServicesContext services_context_;
  DialogService& dialog_service_;
  std::shared_ptr<SettingsStore> settings_store_;

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
