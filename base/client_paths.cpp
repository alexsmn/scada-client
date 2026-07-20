#include "base/client_paths.h"

#include "base/path_service.h"

#include <cstdlib>
#include <filesystem>

namespace client {

namespace {

std::filesystem::path GetHomeDir() {
  if (const char* home = std::getenv("HOME"))
    return home;
  return std::filesystem::temp_directory_path();
}

std::filesystem::path GetInstallDirFromExeDir(std::filesystem::path exe_dir) {
#ifdef __APPLE__
  if (exe_dir.filename() == "MacOS" &&
      exe_dir.parent_path().filename() == "Contents") {
    auto app_dir = exe_dir.parent_path().parent_path();
    if (app_dir.extension() == ".app") {
      return app_dir.parent_path();
    }
  }
#endif

  return exe_dir.parent_path();
}

}  // namespace

bool PathProvider(int key, std::filesystem::path* result) {
  // Assume that we will not need to create the directory if it does not exist.
  // This flag can be set to true for the cases where we want to create it.
  bool create_dir = false;

  std::filesystem::path cur;
  switch (key) {
    case DIR_INSTALL:
      if (!scada::base::PathService::Get(scada::base::DIR_EXE, &cur))
        return false;
      cur = GetInstallDirFromExeDir(std::move(cur));
      create_dir = false;
      break;

    case DIR_DATA:
      if (!scada::base::PathService::Get(DIR_INSTALL, &cur))
        return false;
      cur = cur / "data";
      create_dir = false;
      break;

    case DIR_PUBLIC:
#ifdef _WIN32
      if (!scada::base::PathService::Get(scada::base::DIR_COMMON_APP_DATA,
                                         &cur))
        return false;
      cur = cur / "Telecontrol/SCADA Client";
#else
      cur =
          GetHomeDir() / "Library/Application Support/Telecontrol/SCADA Client";
#endif
      create_dir = true;
      break;

    case DIR_PRIVATE:
#ifdef _WIN32
      if (!scada::base::PathService::Get(scada::base::DIR_APP_DATA, &cur))
        return false;
      cur = cur / "Telecontrol/SCADA Client";
#else
      cur =
          GetHomeDir() / "Library/Application Support/Telecontrol/SCADA Client";
#endif
      create_dir = true;
      break;

    case DIR_DOCUMENTATION:
      if (!scada::base::PathService::Get(client::DIR_INSTALL, &cur))
        return false;
      cur = cur / "docs";
      create_dir = false;
      break;

    case DIR_LOG:
#ifdef _WIN32
      if (!scada::base::PathService::Get(scada::base::DIR_LOCAL_APP_DATA, &cur))
        return false;
      cur = cur / "Telecontrol/SCADA Client/logs";
#else
      cur = GetHomeDir() / "Library/Logs/Telecontrol/SCADA Client";
#endif
      create_dir = true;
      break;

    default:
      return false;
  }

  if (create_dir && !std::filesystem::exists(cur) &&
      !std::filesystem::create_directories(cur))
    return false;

  *result = cur;
  return true;
}

void RegisterPathProvider() {
  scada::base::PathService::RegisterProvider(PathProvider, PATH_START,
                                             PATH_END);
}

}  // namespace client
