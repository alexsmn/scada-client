#include "components/modus/modus_module2.h"

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "client_paths.h"
#include "components/modus/modus_style_library2.h"
#include "libmodus/scheme/master_library.h"

ModusModule2* ModusModule2::s_instance = nullptr;

ModusModule2::ModusModule2() : style_library_(new ModusStyleLibrary2) {
  base::FilePath path;
  PathService::Get(client::DIR_DATA, &path);
  path = path.Append(FILE_PATH_LITERAL("Library.txt"));
  master_library_ = modus::LoadMasterLibrary(path.AsUTF8Unsafe());
}

ModusModule2::~ModusModule2() {}
