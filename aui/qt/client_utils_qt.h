#pragma once

#include "aui/aui_ns_compat.h"

#include <QPixmap>

namespace scada::aui {
class MenuModel;
}

class QMenu;

void BuildMenu(QMenu& menu, aui::MenuModel& model);

QPixmap LoadPixmap(unsigned resource_id);
