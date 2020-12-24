#include "components/modus/libmodus/modus_module2.h"

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "client_paths.h"
#include "components/modus/libmodus/modus_style_library2.h"
#include "libmodus/scheme/master_library.h"

ModusModule2* ModusModule2::s_instance = nullptr;

ModusModule2::ModusModule2(BlinkerManager& blinker_manager)
    : style_library_{std::make_unique<ModusStyleLibrary2>(blinker_manager)} {
  base::FilePath path;
  base::PathService::Get(client::DIR_DATA, &path);
  path = path.Append(FILE_PATH_LITERAL("Library.txt"));
  master_library_ = modus::LoadMasterLibrary(path.AsUTF8Unsafe());
}

ModusModule2::~ModusModule2() {}
