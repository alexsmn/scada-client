#include "modules/login/login_controller.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "base/any_executor_dispatch.h"
#include "base/check.h"
#include "base/client_paths.h"
#include "base/file_settings_store.h"
#include "base/path_service.h"
#include "base/string_util.h"
#include "net/net_executor_adapter.h"
#include "opcua/client/application_pki.h"
#include "opcua/client/endpoint_url.h"

#include "base/u16format.h"
#include "scada/session_service.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <boost/range/iterator_range.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <span>
#include <vector>
#ifdef _WIN32
#include <windows.h>  // for VK_CONTROL
#endif

namespace {

// The session's LocaleIds: the UI language, most preferred, and nothing else.
// A single entry rather than a list because the client has exactly one
// language on screen at a time; an empty UI language becomes an empty list,
// which Part 4 §5.4 reads as "any locale the server has" and is exactly what
// this client wants when it does not know its own.
std::vector<std::string> SessionLocaleIds() {
  std::string locale_name = UiLocaleName();
  if (locale_name.empty())
    return {};
  return {std::move(locale_name)};
}

}  // namespace

namespace {

const wchar_t kRegistryKey[] = L"Software\\Telecontrol\\Workplace";
const char kServerHostKeyPrefix[] = "Host:";

// Backend name (DataServicesInfo::name) of the OPC UA backend. Security
// settings are only meaningful for it.
const char kOpcUaServerType[] = "OpcUa";

// Persisted string values for the security mode, kept stable across releases.
const char kSecurityModeNone[] = "None";
const char kSecurityModeAuto[] = "Auto";
const char kSecurityModeSignAndEncrypt[] = "SignAndEncrypt";

int ParseSecurityModeIndex(std::string_view mode) {
  if (mode == kSecurityModeAuto)
    return 1;
  if (mode == kSecurityModeSignAndEncrypt)
    return 2;
  return 0;
}

std::string_view SecurityModeToString(int index) {
  switch (index) {
    case 1:
      return kSecurityModeAuto;
    case 2:
      return kSecurityModeSignAndEncrypt;
    default:
      return kSecurityModeNone;
  }
}

const char kForceLogoffMessage[] =
    "The specified username is already in use by another session. "
    "Disconnect the open session and continue?";
const wchar_t kLoginFailedMessage[] = L"Error connecting to server ({}).";
// A first meeting with a server's certificate. The fingerprint is what the
// user compares against one the server's operator publishes -- the GCP demo
// serves it at https://<host>/opcua/fingerprint.txt.
const char kTrustCertificateMessage[] =
    "The server presented a certificate this computer has not trusted "
    "before.\n\n{}\nSHA-256 fingerprint:\n{}\n\nCompare the fingerprint "
    "with the one the server's operator publishes. Trust this certificate and "
    "connect?";
// A different certificate from a host already trusted: what an attacker
// between the client and the server would look like.
const char kCertificateChangedMessage[] =
    "WARNING: the server at this address presented a DIFFERENT certificate "
    "from the one this computer trusted before. Someone intercepting the "
    "connection would look exactly like this.\n\n{}\nSHA-256 fingerprint:\n"
    "{}\n\nTrust the new certificate only if the server's operator confirms "
    "that it was replaced. Trust it and connect?";
// The application instance certificate this client generates for itself
// names it by host (OPC UA Part 6 §6.2.2 Application Instance Certificate,
// https://reference.opcfoundation.org/Core/Part6/v105/docs/6.2.2).
const char kApplicationUriPrefix[] = "urn:";
const char kApplicationUriSuffix[] = ":Telecontrol:ScadaClient";
const char kPkiDirectoryKey[] = "OpcUaPkiDirectory";

const char kAutoLoginMessage[] =
    "To disable automatic login, hold Ctrl when launching the application.";

std::string GetServerHostTypeKey(std::string_view server_type_name) {
  return std::format("{}{}", kServerHostKeyPrefix, server_type_name);
}

std::vector<std::string> ParseListString(std::string_view users) {
  std::vector<std::string> result;
  boost::split(result, users, boost::is_any_of(","));
  for (auto& s : result)
    TrimAsciiWhitespace(s);
  std::erase_if(result, [](const auto& s) { return s.empty(); });
  return result;
}

std::vector<std::u16string> ParseListString(std::u16string_view users) {
  std::vector<std::u16string> result;
  boost::split(result, users, boost::is_any_of(u","));
  for (auto& s : result)
    TrimAsciiWhitespace(s);
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

// The per-user client data directory's pki/ (client::DIR_PRIVATE:
// %APPDATA%\Telecontrol\SCADA Client on Windows, ~/Library/Application
// Support/Telecontrol/SCADA Client elsewhere, or --test-data-dir).
std::filesystem::path DefaultPkiDirectory() {
  std::filesystem::path private_dir;
  if (!scada::base::PathService::Get(client::DIR_PRIVATE, &private_dir))
    private_dir = std::filesystem::current_path();
  return private_dir / "pki";
}

// The host part of what the user typed, as a SAN check compares it.
std::string DialledHost(const std::string& server_host) {
  const std::string url = server_host.starts_with("opc.tcp://")
                              ? server_host
                              : "opc.tcp://" + server_host;
  return opcua::ParseOpcTcpUrl(url).host;
}

std::shared_ptr<SettingsStore> CreateDefaultSettingsStore() {
#ifdef _WIN32
  return std::make_shared<RegistrySettingsStore>(HKEY_CURRENT_USER,
                                                 kRegistryKey);
#else
  std::filesystem::path base_dir =
      std::getenv("HOME") ? std::getenv("HOME") : ".";
  return std::make_shared<FileSettingsStore>(
      base_dir /
      "Library/Application Support/Telecontrol/SCADA Client/settings.json");
#endif
}

}  // namespace

LoginController::LoginController(AnyExecutor executor,
                                 DataServicesContext&& services_context,
                                 DialogService& dialog_service,
                                 std::shared_ptr<SettingsStore> settings_store)
    : executor_{std::move(executor)},
      services_context_{std::move(services_context)},
      dialog_service_{dialog_service},
      settings_store_{std::move(settings_store)} {
  if (!settings_store_)
    settings_store_ = CreateDefaultSettingsStore();

  user_name = settings_store_->ReadString16("User");
  std::u16string users = settings_store_->ReadString16("UserList");
  user_list = ParseListString(users);

  auto server_type = settings_store_->ReadString("ServerType");
  password = settings_store_->ReadString16("Password");
  auto_login = settings_store_->ReadBool("AutoLogin");

  security_mode_list = {Translate("No security"),
                        Translate("Most secure available"),
                        Translate("Sign and encrypt")};
  security_mode_index =
      ParseSecurityModeIndex(settings_store_->ReadString("SecurityMode"));
  client_certificate_path = settings_store_->ReadString("ClientCertificate");
  client_private_key_path = settings_store_->ReadString("ClientPrivateKey");
  const auto pki_directory = settings_store_->ReadString(kPkiDirectoryKey);
  pki_directory_ = pki_directory.empty() ? DefaultPkiDirectory()
                                         : std::filesystem::path{pki_directory};

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
  scada::base::Check(!server_type_data_.empty());
  if (server_type_data_[0].host.empty())
    server_type_data_[0].host = settings_store_->ReadString("Host");

  for (size_t i = 0; i < list.size(); ++i) {
    if (server_type_data_[i].host.empty())
      server_type_data_[i].host = list[i].default_host;
  }

  server_host = server_type_data_[server_type_index_].host;

#ifdef _WIN32
  // Don't perform automatic login if Shift is pressed.
  if (GetAsyncKeyState(VK_CONTROL) < 0)
    auto_login = false;
#endif
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

  Dispatch(executor_, weak_from_this(), [this, status] {
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
  settings_store_->Write("SecurityMode",
                         SecurityModeToString(security_mode_index));
  settings_store_->Write("ClientCertificate", client_certificate_path);
  settings_store_->Write("ClientPrivateKey", client_private_key_path);

  // Build the prompt awaitable eagerly, like OnLoginFailed does. Do not wrap
  // the call in a capturing lambda coroutine: the awaitable would read its
  // captures through the temporary closure, which dies at the end of the
  // statement, while the await only happens later inside CompleteLoginAsync.
  std::optional<Awaitable<MessageBoxResult>> message;
  if (auto_login && login_message_) {
    message = dialog_service_.RunMessageBox(Translate(kAutoLoginMessage), {},
                                            MessageBoxMode::Info);
  }

  CoSpawn(executor_,
          [executor = executor_, completion_handler = completion_handler,
           services = std::move(services_),
           message = std::move(message)]() mutable {
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

  if (status.code() == scada::StatusCode::Bad_CertificateUntrusted &&
      PromptToTrustServerCertificate()) {
    return;
  }

  if (status.code() == scada::StatusCode::Bad_UserIsAlreadyLoggedOn) {
    CoSpawn(executor_, [executor = executor_, controller = weak_from_this(),
                        prompt = dialog_service_.RunMessageBox(
                            Translate(kForceLogoffMessage), {},
                            MessageBoxMode::QuestionYesNo)]() mutable {
      return PromptForceLogoffAsync(std::move(executor), std::move(controller),
                                    std::move(prompt));
    });

  } else {
    std::u16string message = u16format(kLoginFailedMessage, ToString16(status));
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

  // A secured OPC UA connect needs an application instance certificate; make
  // one on first use rather than asking the user for PEM files.
  if (IsSecuritySupported() && security_mode_index != 0 &&
      (client_certificate_path.empty() || client_private_key_path.empty())) {
    std::string host_name = boost::asio::ip::host_name();
    opcua::ApplicationPki pki{pki_directory_};
    const auto status = pki.EnsureOwnCertificate(
        std::string{kApplicationUriPrefix} + host_name + kApplicationUriSuffix,
        host_name);
    if (status.bad()) {
      LOG_WARNING(logger_) << "Cannot create the OPC UA client certificate"
                           << LOG_TAG("Directory", pki_directory_.string());
    }
  }

  if (!CreateDataServices(server_type_, services_context_, services_)) {
    LOG_WARNING(logger_) << "Canot create data services";
    OnLoginResult(scada::StatusCode::Bad_UnsupportedProtocolVersion);
    return;
  }

  CoSpawn(executor_, [executor = executor_, controller = weak_from_this(),
                      session_service = services_.session_service_,
                      params = scada::SessionConnectParams{
                          .host = server_host,
                          .user_name = scada::ToLocalizedText(user_name),
                          .password = scada::ToLocalizedText(password),
                          .allow_remote_logoff = allow_remote_logoff,
                          // Ask the server for text in the language this
                          // client is displayed in, so node names and event
                          // messages match the rest of the window rather than
                          // whichever language the configuration was authored
                          // in. OPC UA Part 4 §5.4 Locale Negotiation,
                          // https://reference.opcfoundation.org/Core/Part4/
                          // v105/docs/5.4
                          .locale_ids = SessionLocaleIds(),
                          .security = MakeSecuritySettings()}]() mutable {
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
  // The index originates from UI selection; ignore out-of-range values.
  if (index < 0 || index >= static_cast<int>(server_type_list.size()))
    return;

  if (server_type_index_ == index)
    return;

  server_type_data_[server_type_index_].host = std::move(server_host);
  server_type_index_ = index;
  server_host = server_type_data_[index].host;
}

bool LoginController::IsSecuritySupported() const {
  const auto& list = GetDataServicesInfoList();
  if (server_type_index_ < 0 ||
      server_type_index_ >= static_cast<int>(list.size()))
    return false;
  return EqualDataServicesName(list[server_type_index_].name, kOpcUaServerType);
}

scada::SessionSecuritySettings LoginController::MakeSecuritySettings() const {
  scada::SessionSecuritySettings settings;
  switch (security_mode_index) {
    case 1:
      settings.mode = scada::SessionSecuritySettings::Mode::Auto;
      break;
    case 2:
      settings.mode = scada::SessionSecuritySettings::Mode::SignAndEncrypt;
      break;
    default:
      settings.mode = scada::SessionSecuritySettings::Mode::None;
      break;
  }
  settings.client_certificate_path = client_certificate_path;
  settings.client_private_key_path = client_private_key_path;
  if (settings.mode == scada::SessionSecuritySettings::Mode::None)
    return settings;

  // A secured connect always authenticates the server against this client's
  // own trust list -- it used to trust whatever certificate discovery
  // returned, which is a man in the middle's to choose (OPC UA Part 4 §6.1.3,
  // https://reference.opcfoundation.org/Core/Part4/v105/docs/6.1.3). A refused
  // certificate lands in rejected/, where PromptToTrustServerCertificate()
  // finds it to ask about.
  const opcua::ApplicationPki pki{pki_directory_};
  settings.trusted_certificates_dir = pki.trusted_dir().string();
  settings.rejected_certificates_dir = pki.rejected_dir().string();
  // No certificate configured: use the one this client generated for itself.
  if (settings.client_certificate_path.empty() ||
      settings.client_private_key_path.empty()) {
    settings.client_certificate_path = pki.own_certificate().string();
    settings.client_private_key_path = pki.own_private_key().string();
  }
  return settings;
}

bool LoginController::PromptToTrustServerCertificate() {
  const opcua::ApplicationPki pki{pki_directory_};
  const auto rejected = pki.NewestRejected(DialledHost(server_host));
  if (!rejected)
    return false;
  const std::u16string message = u16format(
      rejected->replaces_trusted ? Translate(kCertificateChangedMessage)
                                 : Translate(kTrustCertificateMessage),
      ToString16(scada::ToLocalizedText(rejected->subject)),
      ToString16(scada::ToLocalizedText(rejected->sha256_fingerprint)));
  CoSpawn(executor_,
          [executor = executor_, controller = weak_from_this(),
           certificate = rejected->file,
           prompt = dialog_service_.RunMessageBox(
               message, {}, MessageBoxMode::QuestionYesNo)]() mutable {
            return PromptTrustServerCertificateAsync(
                std::move(executor), std::move(controller),
                std::move(certificate), std::move(prompt));
          });
  return true;
}

Awaitable<void> LoginController::PromptTrustServerCertificateAsync(
    AnyExecutor executor,
    std::weak_ptr<LoginController> controller,
    std::filesystem::path certificate,
    Awaitable<MessageBoxResult> prompt) {
  try {
    const auto result = co_await std::move(prompt);
    auto controller_ptr = controller.lock();
    if (!controller_ptr)
      co_return;
    if (result == MessageBoxResult::Yes) {
      opcua::ApplicationPki pki{controller_ptr->pki_directory_};
      if (pki.Trust({.file = std::move(certificate)}).good()) {
        controller_ptr->Connect(false);
        co_return;
      }
    }
    controller_ptr->login_message_ = true;
    controller_ptr->error_handler();
  } catch (...) {
  }
  co_return;
}

Awaitable<void> LoginController::ConnectAsync(
    AnyExecutor executor,
    std::weak_ptr<LoginController> controller,
    scada::SessionService& session_service,
    scada::SessionConnectParams params) {
  auto status = co_await session_service.ConnectStatus(std::move(params));
  if (auto controller_ptr = controller.lock()) {
    controller_ptr->OnLoginResult(status);
  }
  co_return;
}

Awaitable<void> LoginController::CompleteLoginAsync(
    AnyExecutor executor,
    std::function<void(DataServices services)> completion_handler,
    DataServices services,
    std::optional<Awaitable<MessageBoxResult>> message) {
  try {
    if (message)
      co_await std::move(*message);
    completion_handler(std::move(services));
  } catch (...) {
  }
  co_return;
}

Awaitable<void> LoginController::PromptForceLogoffAsync(
    AnyExecutor executor,
    std::weak_ptr<LoginController> controller,
    Awaitable<MessageBoxResult> prompt) {
  try {
    auto message_box_result = co_await std::move(prompt);
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
    AnyExecutor executor,
    std::weak_ptr<LoginController> controller,
    Awaitable<MessageBoxResult> prompt) {
  try {
    co_await std::move(prompt);
    if (auto controller_ptr = controller.lock()) {
      controller_ptr->login_message_ = true;
      controller_ptr->error_handler();
    }
  } catch (...) {
  }
  co_return;
}
