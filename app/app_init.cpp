#include "app/app_init.h"

#include "base/boost_log_init.h"
#include "base/client_paths.h"
#include "base/program_options.h"
#include "base/path_service.h"
#include "base/win/dump.h"
#include "common/common_paths.h"

#include <Windows.h>
#include <filesystem>
#include <stdexcept>

namespace {

LONG WINAPI ProcessUnhandledException(_EXCEPTION_POINTERS* exception) {
  auto name = GetDumpFileName("client");

  std::filesystem::path base_path;
  base::PathService::Get(client::DIR_LOG, &base_path);
  auto path = base_path / name;

  DumpException(path.c_str(), *exception);

  return EXCEPTION_EXECUTE_HANDLER;
}

}  // namespace

void InitCrashDump() {
  SetUnhandledExceptionFilter(ProcessUnhandledException);
}

// Path service must be initialized before calling this function.
void InitLogging() {
  std::filesystem::path log_path;
  if (!base::PathService::Get(client::DIR_LOG, &log_path) || log_path.empty()) {
    throw std::runtime_error{"Cannot resolve client log directory"};
  }
  std::filesystem::create_directories(log_path);

  {
    auto path = log_path / "components.log";
    InitBoostLogging({.path = path});
  }
}

AppInit::AppInit() {
  client::InitProgramOptions();

  scada::RegisterPathProvider();
  client::RegisterPathProvider();

  InitCrashDump();
  InitLogging();
}

AppInit::~AppInit() {
  ShutdownBoostLogging();
}
