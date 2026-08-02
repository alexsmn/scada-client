#pragma once

#include "base/any_executor.h"

#include "aui/dialog_service.h"
#include "base/awaitable.h"
#include "base/boost_log.h"
#include "base/settings_store.h"
#include "scada/data_services_factory.h"
#include "scada/localized_text.h"

#include <memory>
#include <optional>

namespace scada {
struct SessionConnectParams;
struct SessionSecuritySettings;
class SessionService;
class Status;
}  // namespace scada

class DialogService;

class LoginController : public std::enable_shared_from_this<LoginController> {
 public:
  LoginController(AnyExecutor executor,
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

  // OPC UA endpoint security selection. Only meaningful for the OPC UA backend
  // (see IsSecuritySupported); other backends ignore it. `security_mode_index`
  // indexes `security_mode_list`: 0 = no security, 1 = most secure available
  // (discovery-driven), 2 = sign and encrypt.
  std::vector<std::u16string> security_mode_list;
  int security_mode_index = 0;
  std::string client_certificate_path;
  std::string client_private_key_path;

  // True when the currently selected backend understands the security settings
  // above (i.e. the OPC UA backend). Lets the dialog show/hide those fields.
  bool IsSecuritySupported() const;

 protected:
  void Connect(bool allow_remote_logoff);

  // Builds the SessionSecuritySettings from `security_mode_index` and the
  // certificate paths.
  scada::SessionSecuritySettings MakeSecuritySettings() const;

  void OnLoginResult(const scada::Status& status);
  void OnLoginCompleted();
  void OnLoginFailed(const scada::Status& status);

  static Awaitable<void> ConnectAsync(AnyExecutor executor,
                                      std::weak_ptr<LoginController> controller,
                                      scada::SessionService& session_service,
                                      scada::SessionConnectParams params);
  static Awaitable<void> CompleteLoginAsync(
      AnyExecutor executor,
      std::function<void(DataServices services)> completion_handler,
      DataServices services,
      std::optional<Awaitable<MessageBoxResult>> message);
  static Awaitable<void> PromptForceLogoffAsync(
      AnyExecutor executor,
      std::weak_ptr<LoginController> controller,
      Awaitable<MessageBoxResult> prompt);
  static Awaitable<void> ReportLoginErrorAsync(
      AnyExecutor executor,
      std::weak_ptr<LoginController> controller,
      Awaitable<MessageBoxResult> prompt);

  const AnyExecutor executor_;
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
