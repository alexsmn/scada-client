#pragma once

#include <functional>

#if defined(UI_VIEWS)
#include "client/controls/views/tree.h"
#elif defined(UI_QT)
#include "client/controls/qt/tree.h"
#endif