#include "app/app_init.h"

#include "aui/translation.h"
#include "base/ui_text.h"
#include "model/node_id_util.h"

#include "base/boost_log_init.h"
#include "base/client_paths.h"
#include "base/path_service.h"
#include "base/program_options.h"
#ifdef _WIN32
#include "base/win/dump.h"
#endif
#include "common/common_paths.h"

#ifdef _WIN32
#include <Windows.h>
#endif
#include <filesystem>
#include <stdexcept>

namespace {

void InitE2eLogPathOverride() {
  auto log_dir = client::GetOptionValue("test-log-dir");
  if (log_dir.empty())
    return;

  std::filesystem::create_directories(log_dir);
  scada::base::PathService::Override(client::DIR_LOG, log_dir);
}

#ifdef _WIN32
LONG WINAPI ProcessUnhandledException(_EXCEPTION_POINTERS* exception) {
  auto name = GetDumpFileName("client");

  std::filesystem::path base_path;
  scada::base::PathService::Get(client::DIR_LOG, &base_path);
  auto path = base_path / name;

  DumpException(path.c_str(), *exception);

  return EXCEPTION_EXECUTE_HANDLER;
}
#endif

}  // namespace

void InitCrashDump() {
#ifdef _WIN32
  SetUnhandledExceptionFilter(ProcessUnhandledException);
#endif
}

// Path service must be initialized before calling this function.
void InitLogging() {
  std::filesystem::path log_path;
  if (!scada::base::PathService::Get(client::DIR_LOG, &log_path) ||
      log_path.empty()) {
    throw std::runtime_error{"Cannot resolve client log directory"};
  }
  std::filesystem::create_directories(log_path);

  {
    auto path = log_path / "components.log";
    InitBoostLogging({.path = path});
  }
}

AppInit::AppInit(int argc, char* argv[]) {
  client::InitProgramOptions(argc, argv);

  scada::RegisterPathProvider();
  scada::RegisterModelNamespaceResolver();
  // Shared formatting code (common/format.h) carries English literals; route
  // them through the client's Qt translation catalogs. Installed here, but
  // only called at display time — after InstalledTranslation loads the .qm.
  scada::SetUiTextTranslator(&Translate);
  client::RegisterPathProvider();
  InitE2eLogPathOverride();

  InitCrashDump();
  InitLogging();
}

AppInit::~AppInit() {
  ShutdownBoostLogging();
}
