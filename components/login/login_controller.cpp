#include "components/login/login_controller.h"

#include "base/executor.h"
#include "base/string_piece_util.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/registry.h"
#include "base/win/registry2.h"
#include "core/session_service.h"
#include "core/status.h"
#include "services/dialog_service.h"

#include <algorithm>

#undef StrCat
#include "base/strings/strcat.h"

namespace {

const wchar_t kRegistryKey[] = L"Software\\Telecontrol\\Workplace";
const char kServerHostKeyPrefix[] = "Host:";

const char16_t kForceLogoffMessage[] =
    u"Указанное имя пользователя уже используется другой сессией. Разорвать "
    u"открытую сессию и продолжить?";
const char16_t kLoginFailedMessage[] =
    u"Ошибка при подключении к серверу (%ls).";
const char16_t kAutoLoginMessage[] =
    u"Чтобы отключить автоматический вход, удерживайте Ctrl при запуске "
    u"приложения.";

std::wstring GetServerHostTypeKey(std::string_view server_type_name) {
  return base::UTF8ToWide(
      base::StrCat({kServerHostKeyPrefix, AsStringPiece(server_type_name)}));
}

}  // namespace

LoginController::LoginController(std::shared_ptr<Executor> executor,
                                 DataServicesContext&& services_context,
                                 DialogService& dialog_service)
    : executor_{std::move(executor)},
      services_context_{std::move(services_context)},
      dialog_service_{dialog_service} {
  Registry reg(HKEY_CURRENT_USER, kRegistryKey, true);
  user_name = base::WideToUTF16(reg.GetString(L"User"));
  std::u16string users = base::WideToUTF16(reg.GetString(L"UserList"));
  for (size_t p = 0; p < users.length();) {
    size_t n = users.find(',', p);
    auto user_name =
        (n == std::u16string::npos) ? users.substr(p) : users.substr(p, n - p);
    if (std::find(user_list.begin(), user_list.end(), user_name) ==
        user_list.end()) {
      user_list.emplace_back(std::move(user_name));
    }
    if (n == std::u16string::npos)
      break;
    p = n + 1;
  }

  auto server_type = base::WideToUTF8(reg.GetString(L"ServerType"));
  password = base::WideToUTF16(reg.GetString(L"Password"));
  auto_login = reg.GetDWORD(L"AutoLogin") != 0;

  auto& list = GetDataServicesInfoList();
  server_type_hosts_.resize(list.size());
  for (size_t i = 0; i < list.size(); ++i) {
    auto& info = list[i];
    server_type_list.emplace_back(info.display_name);
    server_type_hosts_[i] = base::WideToUTF8(
        reg.GetString(GetServerHostTypeKey(info.name).c_str()));
    if (EqualDataServicesName(info.name, server_type))
      SetServerTypeIndex(i);
  }

  // Backward compatibility.
  assert(!server_type_hosts_.empty());
  if (server_type_hosts_[0].empty())
    server_type_hosts_[0] = base::WideToUTF8(reg.GetString(L"Host"));

  for (size_t i = 0; i < list.size(); ++i) {
    if (server_type_hosts_[i].empty())
      server_type_hosts_[i] = list[i].default_host;
  }

  server_host = server_type_hosts_[server_type_index_];

  // Don't perform automatic login if Shift is pressed.
  if (GetAsyncKeyState(VK_CONTROL) < 0)
    auto_login = false;
  login_message_ = !auto_login;
}

void LoginController::Login() {
  server_type_ = GetDataServicesInfoList()[server_type_index_].name;

  Connect(false);
}

void LoginController::OnLoginResult(const scada::Status& status) {
  Dispatch(*executor_, [this, ref = shared_from_this(), status] {
    if (status)
      OnLoginCompleted();
    else
      OnLoginFailed(status);
  });
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

  Registry reg(HKEY_CURRENT_USER, kRegistryKey);
  reg.SetString(L"User", base::UTF16ToWide(user_name).c_str());
  reg.SetString(L"UserList", base::UTF16ToWide(GetUserListString()).c_str());
  reg.SetString(L"Host", base::UTF8ToWide(server_host).c_str());
  reg.SetString(
      GetServerHostTypeKey(GetDataServicesInfoList()[server_type_index_].name)
          .c_str(),
      base::UTF8ToWide(server_host).c_str());
  reg.SetString(L"ServerType", base::UTF8ToWide(server_type_).c_str());
  reg.SetDWORD(L"AutoLogin", auto_login);
  if (auto_login)
    reg.SetString(L"Password", base::UTF16ToWide(password).c_str());

  auto promise = make_resolved_promise(MessageBoxResult::Ok);
  if (auto_login && login_message_) {
    promise = dialog_service_.RunMessageBox(kAutoLoginMessage, {},
                                            MessageBoxMode::Info);
  }

  promise.then([completion_handler = this->completion_handler,
                services = std::move(services_)](MessageBoxResult) mutable {
    completion_handler(std::move(services));
  });
}

void LoginController::OnLoginFailed(const scada::Status& status) {
  services_ = {};

  if (status.code() == scada::StatusCode::Bad_UserIsAlreadyLoggedOn) {
    dialog_service_
        .RunMessageBox(kForceLogoffMessage, {}, MessageBoxMode::QuestionYesNo)
        .then(BindPromiseExecutor(
            executor_, [this, ref = shared_from_this()](
                           MessageBoxResult message_box_result) {
              if (message_box_result == MessageBoxResult::Yes) {
                Connect(true);

              } else {
                login_message_ = true;
                error_handler();
              }
            }));

  } else {
    std::u16string message =
        base::StringPrintf(kLoginFailedMessage, ToString16(status).c_str());
    dialog_service_.RunMessageBox(message, {}, MessageBoxMode::Error)
        .then(BindPromiseExecutor(executor_,
                                  [this, ref = shared_from_this()](
                                      MessageBoxResult message_box_result) {
                                    login_message_ = true;
                                    error_handler();
                                  }));
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

void LoginController::DeleteUserName(std::u16string_view user_name) {
  auto i = std::find(user_list.begin(), user_list.end(), user_name);
  if (i == user_list.end())
    return;

  user_list.erase(i);

  Registry reg(HKEY_CURRENT_USER, kRegistryKey);
  reg.SetString(L"UserList", base::UTF16ToWide(GetUserListString()).c_str());
}

std::u16string LoginController::GetUserListString() const {
  constexpr size_t kMaxCount = 10;
  const size_t count = std::min(kMaxCount, user_list.size());
  const std::vector<std::u16string> truncated_user_list(
      user_list.begin(), user_list.begin() + count);
  return base::JoinString(truncated_user_list, u",");
}

void LoginController::SetServerTypeIndex(int index) {
  assert(index >= 0);
  assert(index < static_cast<int>(server_type_list.size()));

  if (server_type_index_ == index)
    return;

  server_type_hosts_[server_type_index_] = std::move(server_host);
  server_type_index_ = index;
  server_host = server_type_hosts_[index];
}
