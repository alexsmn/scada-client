#pragma once

#if defined(UI_VIEWS)
#include "controls/views/table.h"
#elif defined(UI_QT)
#include "controls/qt/table.h"
#elif defined(UI_WT)
#include "controls/wt/table.h"
#endif
