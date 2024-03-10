#include "app/app_init.h"

#include "base/boost_log_init.h"
#include "base/client_paths.h"
#include "base/command_line.h"
#include "base/file_path_util.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/win/dump.h"
#include "common/common_paths.h"

#include <Windows.h>

namespace {

LONG WINAPI ProcessUnhandledException(_EXCEPTION_POINTERS* exception) {
  auto name = GetDumpFileName("client");

  base::FilePath base_path;
  base::PathService::Get(client::DIR_LOG, &base_path);
  auto path = AsFilesystemPath(base_path) / name;

  DumpException(path.c_str(), *exception);

  return EXCEPTION_EXECUTE_HANDLER;
}

}  // namespace

void InitCrashDump() {
  SetUnhandledExceptionFilter(ProcessUnhandledException);
}

// Path service must be initialized before calling this function.
void InitLogging() {
  base::FilePath log_path;
  base::PathService::Get(client::DIR_LOG, &log_path);
  base::CreateDirectory(log_path);

  {
    auto path = log_path.Append(FILE_PATH_LITERAL("client.log"));
    logging::InitLogging({
        .logging_dest = logging::LOG_TO_FILE,
        .log_file_path = path.value().c_str(),
        .lock_log = logging::LOCK_LOG_FILE,
        .delete_old = logging::APPEND_TO_OLD_LOG_FILE,
    });
  }

  {
    auto path = log_path.Append(FILE_PATH_LITERAL("components.log"));
    InitBoostLogging({.path = path.value()});
  }
}

AppInit::AppInit() {
  if (!base::CommandLine::Init(0, nullptr)) {
    throw std::runtime_error{"Can't parse command line."};
  }

  scada::RegisterPathProvider();
  client::RegisterPathProvider();

  InitCrashDump();
  InitLogging();
}

AppInit::~AppInit() {
  ShutdownBoostLogging();
}
