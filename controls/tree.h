#pragma once

#include <functional>

#if defined(UI_VIEWS)
#include "controls/views/tree.h"
#elif defined(UI_QT)
#include "controls/qt/tree.h"
#endif