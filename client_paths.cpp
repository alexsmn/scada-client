#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "client_paths.h"

namespace client {

bool PathProvider(int key, base::FilePath* result) {
  // Assume that we will not need to create the directory if it does not exist.
  // This flag can be set to true for the cases where we want to create it.
  bool create_dir = false;

  base::FilePath cur;
  switch (key) {
    case DIR_INSTALL:
      if (!base::PathService::Get(base::DIR_EXE, &cur))
        return false;
      cur = cur.DirName();
      create_dir = false;
      break;

    case DIR_DATA:
      if (!base::PathService::Get(DIR_INSTALL, &cur))
        return false;
      cur = cur.Append(FILE_PATH_LITERAL("data"));
      create_dir = false;
      break;

    case DIR_PUBLIC:
      if (!base::PathService::Get(base::DIR_COMMON_APP_DATA, &cur))
        return false;
      cur = cur.Append(FILE_PATH_LITERAL("Telecontrol/SCADA Client"));
      create_dir = true;
      break;

    case DIR_PRIVATE:
      if (!base::PathService::Get(base::DIR_APP_DATA, &cur))
        return false;
      cur = cur.Append(FILE_PATH_LITERAL("Telecontrol/SCADA Client"));
      create_dir = true;
      break;

    case DIR_DOCUMENTATION:
      if (!base::PathService::Get(client::DIR_INSTALL, &cur))
        return false;
      cur = cur.Append(FILE_PATH_LITERAL("docs"));
      create_dir = false;
      break;

    case DIR_LOG:
      if (!base::PathService::Get(base::DIR_LOCAL_APP_DATA, &cur))
        return false;
      cur = cur.Append(FILE_PATH_LITERAL("Telecontrol/SCADA Client/logs"));
      create_dir = true;
      break;

    default:
      return false;
  }

  if (create_dir && !base::PathExists(cur) && !base::CreateDirectory(cur))
    return false;

  *result = cur;
  return true;
}

void RegisterPathProvider() {
  base::PathService::RegisterProvider(PathProvider, PATH_START, PATH_END);
}

} // namespace client
