#pragma once

#include <QPixmap>

namespace ui {
class MenuModel;
}

class QMenu;

void BuildMenu(QMenu& menu, ui::MenuModel& model);

QPixmap LoadPixmap(unsigned resource_id);
