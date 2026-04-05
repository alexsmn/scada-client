#include "components/login/login_controller.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "base/promise_executor.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include "base/u16format.h"
#include "base/utf_convert.h"
#include "base/win/registry.h"
#include "scada/session_service.h"
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

struct RegHelper {
  bool ReadBool(std::string_view name) {
    DWORD value = 0;
    key_.ReadValueDW(UtfConvert<wchar_t>(name).c_str(), &value);
    return value != 0;
  }

  std::string ReadString(std::string_view name) {
    std::wstring value;
    key_.ReadValue(UtfConvert<wchar_t>(name).c_str(), &value);
    return UtfConvert<char>(value);
  }

  std::u16string ReadString16(std::string_view name) {
    std::wstring value;
    key_.ReadValue(UtfConvert<wchar_t>(name).c_str(), &value);
    return UtfConvert<char16_t>(value);
  }

  bool Write(std::string_view name, bool bool_value) {
    DWORD dword_value = bool_value ? 1 : 0;
    return key_.WriteValue(UtfConvert<wchar_t>(name).c_str(), dword_value) ==
           ERROR_SUCCESS;
  }

  bool Write(std::string_view name, std::string_view string_value) {
    return key_.WriteValue(UtfConvert<wchar_t>(name).c_str(),
                           UtfConvert<wchar_t>(string_value).c_str()) ==
           ERROR_SUCCESS;
  }

  bool Write(std::string_view name, std::u16string_view string16_value) {
    return key_.WriteValue(UtfConvert<wchar_t>(name).c_str(),
                           UtfConvert<wchar_t>(string16_value).c_str()) ==
           ERROR_SUCCESS;
  }

  base::win::RegKey& key_;
};

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

void AppendMruList(std::vector<std::u16string>& list,
                   std::u16string_view new_item) {
  if (list.empty() || list.front() != new_item) {
    std::erase(list, new_item);
    list.emplace(list.begin(), new_item);
  }

  if (list.size() > 10)
    list.resize(10);
}

}  // namespace

LoginController::LoginController(std::shared_ptr<Executor> executor,
                                 DataServicesContext&& services_context,
                                 DialogService& dialog_service)
    : executor_{std::move(executor)},
      services_context_{std::move(services_context)},
      dialog_service_{dialog_service} {
  base::win::RegKey reg(HKEY_CURRENT_USER, kRegistryKey, KEY_QUERY_VALUE);
  RegHelper reg_helper{reg};
  user_name = reg_helper.ReadString16("User");
  std::u16string users = reg_helper.ReadString16("UserList");
  user_list = ParseListString(users);

  auto server_type = reg_helper.ReadString("ServerType");
  password = reg_helper.ReadString16("Password");
  auto_login = reg_helper.ReadBool("AutoLogin");

  const auto& list = GetDataServicesInfoList();
  server_type_data_.resize(list.size());
  for (size_t i = 0; i < list.size(); ++i) {
    auto& info = list[i];
    server_type_list.emplace_back(info.display_name);
    server_type_data_[i].host =
        reg_helper.ReadString(GetServerHostTypeKey(info.name));
    if (EqualDataServicesName(info.name, server_type))
      SetServerTypeIndex(i);
  }

  // Backward compatibility.
  assert(!server_type_data_.empty());
  if (server_type_data_[0].host.empty())
    server_type_data_[0].host = reg_helper.ReadString("Host");

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

  base::win::RegKey reg(HKEY_CURRENT_USER, kRegistryKey,
                        KEY_SET_VALUE | KEY_QUERY_VALUE);
  RegHelper reg_helper{reg};
  reg_helper.Write("User", user_name);
  reg_helper.Write("UserList", BuildListString(user_list));
  reg_helper.Write("Host", server_host);
  reg_helper.Write(
      GetServerHostTypeKey(GetDataServicesInfoList()[server_type_index_].name),
      server_host);
  reg_helper.Write("ServerType", server_type_);
  reg_helper.Write("AutoLogin", auto_login);
  if (auto_login)
    reg_helper.Write("Password", password);

  auto promise = make_resolved_promise();
  if (auto_login && login_message_) {
    promise = ToVoidPromise(dialog_service_.RunMessageBox(
        Translate(kAutoLoginMessage), {}, MessageBoxMode::Info));
  }

  promise.then([completion_handler = this->completion_handler,
                services = std::move(services_)]() mutable {
    completion_handler(std::move(services));
  });
}

void LoginController::OnLoginFailed(const scada::Status& status) {
  LOG_WARNING(logger_) << "Login failed" << LOG_TAG("Status", ToString(status));

  services_ = {};

  if (status.code() == scada::StatusCode::Bad_UserIsAlreadyLoggedOn) {
    dialog_service_
        .RunMessageBox(Translate(kForceLogoffMessage), {}, MessageBoxMode::QuestionYesNo)
        .then(BindPromiseExecutor(executor_, weak_from_this(),
                                  [this](MessageBoxResult message_box_result) {
                                    if (message_box_result ==
                                        MessageBoxResult::Yes) {
                                      Connect(true);

                                    } else {
                                      login_message_ = true;
                                      error_handler();
                                    }
                                  }));

  } else {
    std::u16string message =
        u16format(kLoginFailedMessage, ToString16(status));
    dialog_service_.RunMessageBox(message, {}, MessageBoxMode::Error)
        .then(BindPromiseExecutor(executor_, weak_from_this(),
                                  [this](MessageBoxResult) {
                                    login_message_ = true;
                                    error_handler();
                                  }));
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

  scada::BindStatusCallback(
      services_.session_service_->Connect(
          {.host = server_host,
           .user_name = scada::ToLocalizedText(user_name),
           .password = scada::ToLocalizedText(password),
           .allow_remote_logoff = allow_remote_logoff}),
      [this](const scada::Status& status) { OnLoginResult(status); });
}

void LoginController::DeleteUserName(std::u16string_view user_name) {
  auto i = std::ranges::find(user_list, user_name);
  if (i == user_list.end())
    return;

  base::win::RegKey reg(HKEY_CURRENT_USER, kRegistryKey,
                        KEY_SET_VALUE | KEY_QUERY_VALUE);
  RegHelper reg_helper{reg};
  reg_helper.Write("UserList", BuildListString(user_list));
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
