#include "components/login/login_dialog.h"

#include <algorithm>
#include <vector>

#include "base/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/win_util2.h"
#include "base/win/registry2.h"
#include "common_resources.h"
#include "core/data_services.h"
#include "core/data_services_factory.h"
#include "views/client_utils_views.h"
#include "views/framework/dialog.h"
#include "core/session_service.h"
#include "core/status.h"
#include "translation.h"

#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h>

namespace scada {
class SessionService;
}

class LoginDialog : public framework::Dialog {
 public:
  explicit LoginDialog(const DataServicesContext& services_context);
  ~LoginDialog();
  
  const std::string& server_host() const { return server_host_; }
  DataServices& services() { return services_; }

 protected:
  // Dialog
  virtual void OnInitDialog();
  virtual void OnOK();

 private:
  typedef std::vector<std::string> StringList;
  
  void StartLogin();
  void Connect(bool allow_remote_logoff);
  void UpdateControls();

  void OnLoginResult(const scada::Status& status);
  void OnLoginCompleted();
  void OnLoginFailed(const scada::Status& status);

  DataServicesContext services_context_;
  DataServices services_;

  std::string   user_name_;
  std::string   password_;
  std::string   server_host_;
  std::string server_type_;
  StringList    user_list_;
  bool      auto_login_ = false;  // Automatic startup login is performed.
  bool      auto_login_option_ = false;

  bool connecting_ = false;

  base::WeakPtrFactory<LoginDialog> weak_factory_{this};
};

LoginDialog::LoginDialog(const DataServicesContext& services_context)
    : Dialog{IDD_LOGIN},
      services_context_{services_context} {
}

LoginDialog::~LoginDialog() {
}

void LoginDialog::UpdateControls() {
  int ids[] = { IDOK, IDC_NAME, IDC_PASSWORD, IDC_HOST, IDC_SERVER_TYPE, IDC_AUTO_LOGIN };
  bool connecting = connecting_;
  for (int i = 0; i < _countof(ids); i++)
    EnableWindow(GetItem(ids[i]), !connecting);

  SetItemText(IDC_STATUS, L"");
}

void LoginDialog::OnInitDialog() {
  Registry reg(HKEY_CURRENT_USER, L"Software\\Telecontrol\\Workplace");
  std::string user_name = base::SysWideToNativeMB(reg.GetString(L"User"));
  std::string user_list = base::SysWideToNativeMB(reg.GetString(L"UserList"));
  std::string host = base::SysWideToNativeMB(reg.GetString(L"Host"));
  std::string server_type = base::SysWideToNativeMB(reg.GetString(L"ServerType"));
  std::string password = base::SysWideToNativeMB(reg.GetString(L"Password"));
  auto_login_ = reg.GetDWORD(L"AutoLogin") != 0;

  WTL::CButton(GetItem(IDC_AUTO_LOGIN)).SetCheck(auto_login_ ? BST_CHECKED : BST_UNCHECKED);

  // Don't perform automatic login if Shift is pressed. 
  if (GetAsyncKeyState(VK_CONTROL) < 0)
    auto_login_ = false;

  // Users.
  {
    WTL::CComboBox user_combo = GetItem(IDC_NAME);
    for (size_t p = 0; p < user_list.length(); ) {
      size_t n = user_list.find(',', p);
      std::string user_name = (n == std::string::npos) ? user_list.substr(p) :
                                                         user_list.substr(p, n - p);
      if (std::find(user_list_.begin(), user_list_.end(), user_name) ==
              user_list_.end()) {
        user_list_.push_back(user_name);
        user_combo.AddString(base::SysNativeMBToWide(user_name).c_str());
      }
      if (n == std::string::npos)
        break;
      p = n + 1;
    }
    user_combo.SetWindowText(base::SysNativeMBToWide(user_name).c_str());
  }

  SetItemText(IDC_HOST, base::SysNativeMBToWide(host));

  // Server type.
  {
    WTL::CComboBox server_type_combo = GetItem(IDC_SERVER_TYPE);
    int selected_index = 0;
    auto& list = GetDataServicesInfoList();
    for (size_t i = 0; i < list.size(); ++i) {
      auto& info = list[i];
      server_type_combo.AddString(info.display_name.c_str());
      if (EqualDataServicesName(info.name, server_type))
        selected_index = i;
    }
    server_type_combo.SetCurSel(selected_index);
  }

  if (auto_login_) {
    SetItemText(IDC_PASSWORD, base::SysNativeMBToWide(password));
    StartLogin();
  }

  UpdateControls();
}

void LoginDialog::StartLogin() {
  assert(!false);

  user_name_ = base::SysWideToNativeMB(GetItemText(IDC_NAME));
  password_ = base::SysWideToNativeMB(GetItemText(IDC_PASSWORD));
  server_host_ = base::SysWideToNativeMB(GetItemText(IDC_HOST));
  {
    int server_type_index = WTL::CComboBox(GetItem(IDC_SERVER_TYPE)).GetCurSel();
    server_type_index = std::max(server_type_index, 0);
    server_type_ = GetDataServicesInfoList()[server_type_index].name;
  }
  auto_login_option_ = WTL::CButton(GetItem(IDC_AUTO_LOGIN)).GetState() == BST_CHECKED;

  Connect(false);
  UpdateControls();

  SetItemText(IDC_STATUS, L"Подключение...");
}

void LoginDialog::OnOK() {
  auto_login_ = false;
  StartLogin();
}

void LoginDialog::OnLoginResult(const scada::Status& status) {
  connecting_ = false;

  base::Closure task;
  if (status)
    task = base::Bind(&LoginDialog::OnLoginCompleted, weak_factory_.GetWeakPtr());
  else
    task = base::Bind(&LoginDialog::OnLoginFailed, weak_factory_.GetWeakPtr(), status);

  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, task);
}

void LoginDialog::OnLoginCompleted() {
  // save last users
  StringList::iterator i = std::find(user_list_.begin(), user_list_.end(),
                                     user_name_);
  if (i == user_list_.end())
    user_list_.push_back(user_name_);
  while (user_list_.size() > 10)
    user_list_.erase(user_list_.begin());

  std::string user_list;
  for (StringList::iterator i = user_list_.begin(); i != user_list_.end(); i++) {
    if (!user_list.empty())
      user_list += ',';
    user_list += *i;
  }

  Registry reg(HKEY_CURRENT_USER, L"Software\\Telecontrol\\Workplace");
  reg.SetString(L"User", base::SysNativeMBToWide(user_name_).c_str());
  reg.SetString(L"UserList", base::SysNativeMBToWide(user_list).c_str());
  reg.SetString(L"Host", base::SysNativeMBToWide(server_host_).c_str());
  reg.SetString(L"ServerType", base::SysNativeMBToWide(server_type_).c_str());
  reg.SetDWORD(L"AutoLogin", auto_login_option_);
  if (auto_login_option_)
    reg.SetString(L"Password", base::SysNativeMBToWide(password_).c_str());

  if (!auto_login_ && auto_login_option_) {
    MessageBoxW(::GetActiveWindow(),
        L"Для того, чтобы отключить автоматический вход, удерживайте Ctrl при запуске приложения.",
        L"Автоматический вход", MB_ICONINFORMATION | MB_OK);
  }

  EndDialog(IDOK);
}

void LoginDialog::OnLoginFailed(const scada::Status& status) {
  services_ = {};

  base::string16 title = win_util::GetWindowText(window_handle());

  if (status.code() == scada::StatusCode::Bad_UserIsAlreadyLoggedOn) {
    const wchar_t* msg = L"Указанное имя уже используется другой сессией. "
                         L"Разорвать открытую сессию и продолжить?";
    if (MessageBoxW(window_handle(), msg, title.c_str(),
                    MB_YESNO | MB_ICONQUESTION) == IDYES) {
      Connect(true);
      
    } else {
      UpdateControls();
    }

  } else {
    base::string16 msg = base::StringPrintf(
        L"Ошибка при подключении к серверу (%ls).",
        Translate(status.ToString()).c_str());
    MessageBoxW(window_handle(), msg.c_str(), title.c_str(), MB_OK | MB_ICONSTOP);

    UpdateControls();
    WTL::CEdit edit = GetItem(IDC_NAME);
    edit.SetSelAll();
    edit.SetFocus();
  }
}

/*void LoginDialog::OnRequestData(RequestHandle handle, void* data) {
  WTL::CStatic status_window = GetItem(IDC_STATUS);
  status_window.SetWindowText("Получение данных...");
  status_window.UpdateWindow();
}*/

void LoginDialog::Connect(bool allow_remote_logoff) {
  connecting_ = true;

  if (!CreateDataServices(server_type_, services_context_, services_)) {
    OnLoginResult(scada::StatusCode::Bad_UnsupportedProtocolVersion);
    return;
  }

  services_.session_service_->Connect(server_host_, user_name_, password_, allow_remote_logoff,
      [this](const scada::Status& status) { OnLoginResult(status); });
}

bool ExecuteLoginDialog(const DataServicesContext& services_context, DataServices& services) {
  LoginDialog login_dialog{services_context};
  if (login_dialog.Execute() != IDOK)
    return false;
  services = login_dialog.services();
  return true;
}
