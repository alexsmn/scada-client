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

}  // namespace

bool PathProvider(int key, std::filesystem::path* result) {
  // Assume that we will not need to create the directory if it does not exist.
  // This flag can be set to true for the cases where we want to create it.
  bool create_dir = false;

  std::filesystem::path cur;
  switch (key) {
    case DIR_INSTALL:
      if (!base::PathService::Get(base::DIR_EXE, &cur))
        return false;
      cur = cur.parent_path();
      create_dir = false;
      break;

    case DIR_DATA:
      if (!base::PathService::Get(DIR_INSTALL, &cur))
        return false;
      cur = cur / "data";
      create_dir = false;
      break;

    case DIR_PUBLIC:
#ifdef _WIN32
      if (!base::PathService::Get(base::DIR_COMMON_APP_DATA, &cur))
        return false;
      cur = cur / "Telecontrol/SCADA Client";
#else
      cur = GetHomeDir() / "Library/Application Support/Telecontrol/SCADA Client";
#endif
      create_dir = true;
      break;

    case DIR_PRIVATE:
#ifdef _WIN32
      if (!base::PathService::Get(base::DIR_APP_DATA, &cur))
        return false;
      cur = cur / "Telecontrol/SCADA Client";
#else
      cur = GetHomeDir() / "Library/Application Support/Telecontrol/SCADA Client";
#endif
      create_dir = true;
      break;

    case DIR_DOCUMENTATION:
      if (!base::PathService::Get(client::DIR_INSTALL, &cur))
        return false;
      cur = cur / "docs";
      create_dir = false;
      break;

    case DIR_LOG:
#ifdef _WIN32
      if (!base::PathService::Get(base::DIR_LOCAL_APP_DATA, &cur))
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
  base::PathService::RegisterProvider(PathProvider, PATH_START, PATH_END);
}

}  // namespace client
