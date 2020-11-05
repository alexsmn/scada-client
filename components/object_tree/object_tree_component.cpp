#include "components/object_tree/object_tree_view.h"
#include "controller_factory.h"

const WindowInfo kWindowInfo = {
    ID_OBJECT_VIEW, "Struct", L"Объекты", WIN_SING, 200, 400, 0};

REGISTER_CONTROLLER(ObjectTreeView, kWindowInfo);
