#include "components/login/login_controller.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "base/awaitable_promise.h"
#include "net/net_executor_adapter.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include "base/u16format.h"
#include "scada/session_service.h"
#include "scada/status_exception.h"
#include "scada/status_promise.h"

#include <algorithm>
#include <format>
#include <span>
#include <windows.h>  // for VK_CONTROL

namespace {

const wchar_t kRegistryKey[] = L"Software\\Telecontrol\\Workplace";
const char kServerHostKeyPrefix[] = "Host:";

const char kForceLogoffMessage[] =
    "The specified username is already in use by another session. "
    "Disconnect the open session and continue?";
const wchar_t kLoginFailedMessage[] =
    L"Error connecting to server ({}).";
const char kAutoLoginMessage[] =
    "To disable automatic login, hold Ctrl when launching the application.";

std::string GetServerHostTypeKey(std::string_view server_type_name) {
  return std::format("{}{}", kServerHostKeyPrefix, server_type_name);
}

std::vector<std::string> ParseListString(std::string_view users) {
  std::vector<std::string> result;
  boost::split(result, users, boost::is_any_of(","));
  for (auto& s : result)
    boost::trim(s);
  std::erase_if(result, [](const auto& s) { return s.empty(); });
  return result;
}

std::vector<std::u16string> ParseListString(std::u16string_view users) {
  std::vector<std::u16string> result;
  boost::split(result, users, boost::is_any_of(u","));
  for (auto& s : result)
    boost::trim(s);
  std::erase_if(result, [](const auto& s) { return s.empty(); });
  return result;
}

std::string BuildListString(std::span<const std::string> list) {
  constexpr size_t kMaxCount = 10;
  auto sublist = list.subspan(0, std::min(kMaxCount, list.size()));
  return boost::algorithm::join(
      boost::make_iterator_range(sublist.begin(), sublist.end()), ",");
}

std::u16string BuildListString(std::span<const std::u16string> list) {
  constexpr size_t kMaxCount = 10;
  auto sublist = list.subspan(0, std::min(kMaxCount, list.size()));
  return boost::algorithm::join(
      boost::make_iterator_range(sublist.begin(), sublist.end()), u",");
}

std::u16string TranslateDataServicesDisplayName(
    std::u16string_view display_name) {
  if (display_name == u"Telecontrol")
    return Translate("Telecontrol");
  if (display_name == u"Vidicon")
    return Translate("Vidicon");
  return std::u16string{display_name};
}

void AppendMruList(std::vector<std::u16string>& list,
                   std::u16string_view new_item) {
  if (list.empty() || list.front() != new_item) {
    std::erase(list, new_item);
    list.emplace(list.begin(), new_item);
  }

  if (list.size() > 10)
    list.resize(10);
}

Awaitable<scada::Status> AwaitStatus(std::shared_ptr<Executor> executor,
                                      promise<void> operation) {
  try {
    co_await AwaitPromise(NetExecutorAdapter{std::move(executor)},
                          std::move(operation));
    co_return scada::StatusCode::Good;
  } catch (...) {
    co_return scada::GetExceptionStatus(std::current_exception());
  }
}

}  // namespace

LoginController::LoginController(std::shared_ptr<Executor> executor,
                                 DataServicesContext&& services_context,
                                 DialogService& dialog_service,
                                 std::shared_ptr<SettingsStore> settings_store)
    : executor_{std::move(executor)},
      services_context_{std::move(services_context)},
      dialog_service_{dialog_service},
      settings_store_{std::move(settings_store)} {
  if (!settings_store_)
    settings_store_ =
        std::make_shared<RegistrySettingsStore>(HKEY_CURRENT_USER, kRegistryKey);

  user_name = settings_store_->ReadString16("User");
  std::u16string users = settings_store_->ReadString16("UserList");
  user_list = ParseListString(users);

  auto server_type = settings_store_->ReadString("ServerType");
  password = settings_store_->ReadString16("Password");
  auto_login = settings_store_->ReadBool("AutoLogin");

  const auto& list = GetDataServicesInfoList();
  server_type_data_.resize(list.size());
  for (size_t i = 0; i < list.size(); ++i) {
    auto& info = list[i];
    server_type_list.emplace_back(
        TranslateDataServicesDisplayName(info.display_name));
    server_type_data_[i].host =
        settings_store_->ReadString(GetServerHostTypeKey(info.name));
    if (EqualDataServicesName(info.name, server_type))
      SetServerTypeIndex(i);
  }

  // Backward compatibility.
  assert(!server_type_data_.empty());
  if (server_type_data_[0].host.empty())
    server_type_data_[0].host = settings_store_->ReadString("Host");

  for (size_t i = 0; i < list.size(); ++i) {
    if (server_type_data_[i].host.empty())
      server_type_data_[i].host = list[i].default_host;
  }

  server_host = server_type_data_[server_type_index_].host;

  // Don't perform automatic login if Shift is pressed.
  if (GetAsyncKeyState(VK_CONTROL) < 0)
    auto_login = false;
  login_message_ = !auto_login;
}

void LoginController::Login() {
  LOG_INFO(logger_) << "Login";

  server_type_ = GetDataServicesInfoList()[server_type_index_].name;

  Connect(false);
}

void LoginController::OnLoginResult(const scada::Status& status) {
  LOG_INFO(logger_) << "Connect completed"
                    << LOG_TAG("Status", ToString(status));

  Dispatch(*executor_, weak_from_this(), [this, status] {
    if (status)
      OnLoginCompleted();
    else
      OnLoginFailed(status);
  });
}

void LoginController::OnLoginCompleted() {
  LOG_INFO(logger_) << "Login completed";

  // save last users
  AppendMruList(user_list, user_name);

  settings_store_->Write("User", user_name);
  settings_store_->Write("UserList", BuildListString(user_list));
  settings_store_->Write("Host", server_host);
  settings_store_->Write(
      GetServerHostTypeKey(GetDataServicesInfoList()[server_type_index_].name),
      server_host);
  settings_store_->Write("ServerType", server_type_);
  settings_store_->Write("AutoLogin", auto_login);
  if (auto_login)
    settings_store_->Write("Password", password);

  auto promise = make_resolved_promise();
  if (auto_login && login_message_) {
    promise = ToVoidPromise(dialog_service_.RunMessageBox(
        Translate(kAutoLoginMessage), {}, MessageBoxMode::Info));
  }

  CoSpawn(executor_, [executor = executor_,
                      completion_handler = completion_handler,
                      services = std::move(services_),
                      message = std::move(promise)]() mutable {
    return CompleteLoginAsync(std::move(executor),
                              std::move(completion_handler),
                              std::move(services), std::move(message));
  });
}

void LoginController::OnLoginFailed(const scada::Status& status) {
  LOG_WARNING(logger_) << "Login failed" << LOG_TAG("Status", ToString(status));

  services_ = {};

  if (login_failed_handler && login_failed_handler(status))
    return;

  if (status.code() == scada::StatusCode::Bad_UserIsAlreadyLoggedOn) {
    CoSpawn(executor_, [executor = executor_, controller = weak_from_this(),
                        prompt = dialog_service_.RunMessageBox(
                            Translate(kForceLogoffMessage), {},
                            MessageBoxMode::QuestionYesNo)]() mutable {
      return PromptForceLogoffAsync(std::move(executor), std::move(controller),
                                    std::move(prompt));
    });

  } else {
    std::u16string message =
        u16format(kLoginFailedMessage, ToString16(status));
    CoSpawn(executor_, [executor = executor_, controller = weak_from_this(),
                        prompt = dialog_service_.RunMessageBox(
                            message, {}, MessageBoxMode::Error)]() mutable {
      return ReportLoginErrorAsync(std::move(executor), std::move(controller),
                                   std::move(prompt));
    });
  }
}

void LoginController::Connect(bool allow_remote_logoff) {
  LOG_INFO(logger_) << "Connect"
                    << LOG_TAG("AllowRemoteLogoff", allow_remote_logoff);

  connecting_ = true;

  if (!CreateDataServices(server_type_, services_context_, services_)) {
    LOG_WARNING(logger_) << "Canot create data services";
    OnLoginResult(scada::StatusCode::Bad_UnsupportedProtocolVersion);
    return;
  }

  CoSpawn(executor_,
          [executor = executor_, controller = weak_from_this(),
           session_service = services_.session_service_,
           params = scada::SessionConnectParams{
               .host = server_host,
               .user_name = scada::ToLocalizedText(user_name),
               .password = scada::ToLocalizedText(password),
               .allow_remote_logoff = allow_remote_logoff}]() mutable {
            return ConnectAsync(std::move(executor), std::move(controller),
                                *session_service, std::move(params));
          });
}

void LoginController::DeleteUserName(std::u16string_view user_name) {
  auto i = std::ranges::find(user_list, user_name);
  if (i == user_list.end())
    return;

  settings_store_->Write("UserList", BuildListString(user_list));
}

void LoginController::SetServerTypeIndex(int index) {
  assert(index >= 0);
  assert(index < static_cast<int>(server_type_list.size()));

  if (server_type_index_ == index)
    return;

  server_type_data_[server_type_index_].host = std::move(server_host);
  server_type_index_ = index;
  server_host = server_type_data_[index].host;
}

Awaitable<void> LoginController::ConnectAsync(
    std::shared_ptr<Executor> executor,
    std::weak_ptr<LoginController> controller,
    scada::SessionService& session_service,
    scada::SessionConnectParams params) {
  auto status = co_await AwaitStatus(std::move(executor),
                                     session_service.Connect(params));
  if (auto controller_ptr = controller.lock()) {
    controller_ptr->OnLoginResult(status);
  }
  co_return;
}

Awaitable<void> LoginController::CompleteLoginAsync(
    std::shared_ptr<Executor> executor,
    std::function<void(DataServices services)> completion_handler,
    DataServices services,
    promise<void> message) {
  try {
    co_await AwaitPromise(NetExecutorAdapter{std::move(executor)},
                          std::move(message));
    completion_handler(std::move(services));
  } catch (...) {
  }
  co_return;
}

Awaitable<void> LoginController::PromptForceLogoffAsync(
    std::shared_ptr<Executor> executor,
    std::weak_ptr<LoginController> controller,
    promise<MessageBoxResult> prompt) {
  try {
    auto message_box_result =
        co_await AwaitPromise(NetExecutorAdapter{std::move(executor)},
                              std::move(prompt));
    if (auto controller_ptr = controller.lock()) {
      if (message_box_result == MessageBoxResult::Yes) {
        controller_ptr->Connect(true);
      } else {
        controller_ptr->login_message_ = true;
        controller_ptr->error_handler();
      }
    }
  } catch (...) {
  }
  co_return;
}

Awaitable<void> LoginController::ReportLoginErrorAsync(
    std::shared_ptr<Executor> executor,
    std::weak_ptr<LoginController> controller,
    promise<MessageBoxResult> prompt) {
  try {
    co_await AwaitPromise(NetExecutorAdapter{std::move(executor)},
                          std::move(prompt));
    if (auto controller_ptr = controller.lock()) {
      controller_ptr->login_message_ = true;
      controller_ptr->error_handler();
    }
  } catch (...) {
  }
  co_return;
}
