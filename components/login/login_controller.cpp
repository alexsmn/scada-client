#include "components/login/login_controller.h"

#include "base/bind.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/registry2.h"
#include "core/session_service.h"
#include "core/status.h"
#include "services/dialog_service.h"

#include <algorithm>

namespace {

const wchar_t kRegistryKey[] = L"Software\\Telecontrol\\Workplace";

const wchar_t kForceLogoffMessage[] =
    L"Указанное имя пользователя уже используется другой сессией. Разорвать "
    L"открытую сессию и продолжить?";
const wchar_t kLoginFailedMessage[] =
    L"Ошибка при подключении к серверу (%ls).";
const wchar_t kAutoLoginMessage[] =
    L"Чтобы отключить автоматический вход, удерживайте Ctrl при запуске "
    L"приложения.";

}  // namespace

LoginController::LoginController(DataServicesContext&& services_context,
                                 DialogService& dialog_service)
    : services_context_{std::move(services_context)},
      dialog_service_{dialog_service} {
  Registry reg(HKEY_CURRENT_USER, kRegistryKey);
  user_name = reg.GetString(L"User");
  std::wstring users = reg.GetString(L"UserList");
  for (size_t p = 0; p < users.length();) {
    size_t n = users.find(',', p);
    auto user_name =
        (n == std::wstring::npos) ? users.substr(p) : users.substr(p, n - p);
    if (std::find(user_list.begin(), user_list.end(), user_name) ==
        user_list.end()) {
      user_list.emplace_back(std::move(user_name));
    }
    if (n == std::wstring::npos)
      break;
    p = n + 1;
  }

  server_host = base::SysWideToNativeMB(reg.GetString(L"Host"));
  auto server_type = base::SysWideToNativeMB(reg.GetString(L"ServerType"));
  password = reg.GetString(L"Password");
  auto_login = reg.GetDWORD(L"AutoLogin") != 0;

  auto& list = GetDataServicesInfoList();
  for (size_t i = 0; i < list.size(); ++i) {
    auto& info = list[i];
    server_type_list.emplace_back(info.display_name);
    if (EqualDataServicesName(info.name, server_type))
      server_type_index = i;
  }

  // Don't perform automatic login if Shift is pressed.
  if (GetAsyncKeyState(VK_CONTROL) < 0)
    auto_login = false;
  login_message_ = !auto_login;
}

void LoginController::Login() {
  server_type_ = GetDataServicesInfoList()[server_type_index].name;

  Connect(false);
}

void LoginController::OnLoginResult(const scada::Status& status) {
  connecting_ = false;

  base::Closure task;
  if (status)
    task = base::Bind(&LoginController::OnLoginCompleted,
                      weak_factory_.GetWeakPtr());
  else
    task = base::Bind(&LoginController::OnLoginFailed,
                      weak_factory_.GetWeakPtr(), status);

  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, task);
}

void LoginController::OnLoginCompleted() {
  auto i = std::find(user_list.begin(), user_list.end(), user_name);
  if (i == user_list.end())
    user_list.emplace_back(user_name);

  Registry reg(HKEY_CURRENT_USER, kRegistryKey);
  reg.SetString(L"User", ToString16(user_name).c_str());
  reg.SetString(L"UserList", GetUserListString().c_str());
  reg.SetString(L"Host", base::SysNativeMBToWide(server_host).c_str());
  reg.SetString(L"ServerType", base::SysNativeMBToWide(server_type_).c_str());
  reg.SetDWORD(L"AutoLogin", auto_login);
  if (auto_login)
    reg.SetString(L"Password", password.c_str());

  if (auto_login && login_message_)
    dialog_service_.RunMessageBox(kAutoLoginMessage, {}, MessageBoxMode::Info);

  completion_handler(services_);
}

void LoginController::OnLoginFailed(const scada::Status& status) {
  services_ = {};

  if (status.code() == scada::StatusCode::Bad_UserIsAlreadyLoggedOn) {
    if (dialog_service_.RunMessageBox(kForceLogoffMessage, {},
                                      MessageBoxMode::QuestionYesNo) ==
        MessageBoxResult::Yes) {
      Connect(true);

    } else {
      login_message_ = true;
      error_handler();
    }

  } else {
    std::wstring message =
        base::StringPrintf(kLoginFailedMessage, ToString16(status).c_str());
    dialog_service_.RunMessageBox(message, {}, MessageBoxMode::Error);

    login_message_ = true;
    error_handler();
  }
}

void LoginController::Connect(bool allow_remote_logoff) {
  connecting_ = true;

  if (!CreateDataServices(server_type_, services_context_, services_)) {
    OnLoginResult(scada::StatusCode::Bad_UnsupportedProtocolVersion);
    return;
  }

  services_.session_service_->Connect(
      server_host, scada::ToLocalizedText(user_name),
      scada::ToLocalizedText(password), allow_remote_logoff,
      [this](const scada::Status& status) { OnLoginResult(status); });
}

void LoginController::DeleteUserName(base::StringPiece16 user_name) {
  auto i = std::find(user_list.begin(), user_list.end(), user_name);
  if (i == user_list.end())
    return;

  user_list.erase(i);

  Registry reg(HKEY_CURRENT_USER, kRegistryKey);
  reg.SetString(L"UserList", GetUserListString().c_str());
}

std::wstring LoginController::GetUserListString() const {
  constexpr size_t kMaxCount = 10;
  const size_t count = std::min(kMaxCount, user_list.size());
  const std::vector<std::wstring> truncated_user_list(
      user_list.begin(), user_list.begin() + count);
  return base::JoinString(truncated_user_list, L",");
}
