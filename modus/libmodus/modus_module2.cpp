#include "modus/libmodus/modus_module2.h"

#include "base/client_paths.h"
#include "base/path_service.h"
#include "modus/libmodus/modus_style_library2.h"
#include "libmodus/scheme/master_library.h"

#include <filesystem>

ModusModule2* ModusModule2::s_instance = nullptr;

ModusModule2::ModusModule2(BlinkerManager& blinker_manager)
    : style_library_{std::make_unique<ModusStyleLibrary2>(blinker_manager)} {
  std::filesystem::path path;
  base::PathService::Get(client::DIR_DATA, &path);
  path /= "Library.txt";
  master_library_ = std::make_unique<modus::MasterLibrary>();
  modus::LoadMasterLibrary(*master_library_, path.string());
}

ModusModule2::~ModusModule2() {}
