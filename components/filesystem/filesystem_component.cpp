#include "components/filesystem/filesystem_view.h"
#include "controller_registry.h"

const WindowInfo kWindowInfo = {
    ID_FILE_SYSTEM_VIEW, "FileSystemView", L"Файлы", WIN_SING, 200, 400};

REGISTER_CONTROLLER(FileSystemView, kWindowInfo);
