#pragma once

#include <QPixmap>

namespace aui {
class MenuModel;
}

class QMenu;

void BuildMenu(QMenu& menu, aui::MenuModel& model);

QPixmap LoadPixmap(unsigned resource_id);
