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
  base::string16 users = reg.GetString(L"UserList");
  for (size_t p = 0; p < users.length();) {
    size_t n = users.find(',', p);
    auto user_name =
        (n == base::string16::npos) ? users.substr(p) : users.substr(p, n - p);
    if (std::find(user_list.begin(), user_list.end(), user_name) ==
        user_list.end()) {
      user_list.emplace_back(std::move(user_name));
    }
    if (n == base::string16::npos)
      break;
    p = n + 1;
  }
  server_host = base::SysWideToNativeMB(reg.GetString(L"Host"));
  server_type = base::SysWideToNativeMB(reg.GetString(L"ServerType"));
  password = reg.GetString(L"Password");
  auto_login = reg.GetDWORD(L"AutoLogin") != 0;
  // Don't perform automatic login if Shift is pressed.
  if (GetAsyncKeyState(VK_CONTROL) < 0)
    auto_login = false;
  login_message_ = !auto_login;
}

void LoginController::Login() {
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
  // save last users
  auto i = std::find(user_list.begin(), user_list.end(), user_name);
  if (i == user_list.end())
    user_list.emplace_back(user_name);
  if (user_list.size() > 10) {
    user_list.erase(user_list.begin(),
                    user_list.begin() + user_list.size() - 10);
  }

  base::string16 user_list_string = base::JoinString(user_list, L",");

  Registry reg(HKEY_CURRENT_USER, kRegistryKey);
  reg.SetString(L"User", ToString16(user_name).c_str());
  reg.SetString(L"UserList", user_list_string.c_str());
  reg.SetString(L"Host", base::SysNativeMBToWide(server_host).c_str());
  reg.SetString(L"ServerType", base::SysNativeMBToWide(server_type).c_str());
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
    base::string16 message =
        base::StringPrintf(kLoginFailedMessage, ToString16(status).c_str());
    dialog_service_.RunMessageBox(message, {}, MessageBoxMode::Error);

    login_message_ = true;
    error_handler();
  }
}

void LoginController::Connect(bool allow_remote_logoff) {
  connecting_ = true;

  if (!CreateDataServices(server_type, services_context_, services_)) {
    OnLoginResult(scada::StatusCode::Bad_UnsupportedProtocolVersion);
    return;
  }

  services_.session_service_->Connect(
      server_host, scada::ToLocalizedText(user_name), scada::ToLocalizedText(password),
      allow_remote_logoff,
      [this](const scada::Status& status) { OnLoginResult(status); });
}
