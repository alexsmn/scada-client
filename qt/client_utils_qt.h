#pragma once

namespace ui {
class MenuModel;
}

class QMenu;

void BuildMenu(QMenu& menu, ui::MenuModel& model);
