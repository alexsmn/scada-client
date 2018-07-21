#include "window_info.h"

#include <cassert>
#include <cstdlib>
#include <exception>

#include "base/compiler_specific.h"
#include "common_resources.h"

const WindowInfo g_window_infos[] = {
  { ID_NEW_PROPERTY_VIEW, "NewProps", L"Свойства", WIN_DISALLOW_NEW | WIN_REQUIRES_ADMIN, 200, 400 },
  { ID_CELLS_VIEW, "Cells", L"Ячейки", WIN_INS | WIN_CAN_PRINT, 0, 0, 0 },
  { ID_SHEET_VIEW, "CusTable", L"Пользовательская таблица", WIN_INS | WIN_CAN_PRINT, 0, 0, IDR_SHEET_POPUP },
  { ID_EVENT_VIEW, "Event", L"События", WIN_SING | WIN_DOCKB | WIN_CAN_PRINT, 800, 200, 0 },
  { ID_EVENT_JOURNAL_VIEW, "EventJournal", L"Журнал событий", WIN_INS | WIN_CAN_PRINT, 0, 0, 0 },
  { ID_FAVOURITES_VIEW, "Favorites", L"Избранное", WIN_SING, 200, 400 },
  { ID_TS_FORMATS_VIEW, "Params", L"Форматы", WIN_REQUIRES_ADMIN, 0, 0, IDR_GRID_POPUP },
  { ID_GRAPH_VIEW, "Graph", L"График", WIN_INS | WIN_CAN_PRINT, 0, 0, IDR_GRAPH_POPUP },
  { ID_WATCH_VIEW, "Log", L"Наблюдение", WIN_DISALLOW_NEW, 0, 0, 0 },
  { ID_MODUS_VIEW, "Modus", L"Схема", WIN_CAN_PRINT, 0, 0, IDR_MODUS_POPUP },
  { ID_PORTFOLIO_VIEW, "Portfolio", L"Портфолио", WIN_SING | WIN_INS, 200, 400, 0 },
  { ID_OBJECT_VIEW, "Struct", L"Объекты", WIN_SING, 200, 400, 0 },
  { ID_TABLE_VIEW, "Table", L"Таблица", WIN_INS | WIN_CAN_PRINT, 620, 400 },
  { ID_SIMULATION_ITEMS_VIEW, "SimulationItems", L"Эмулируемые сигналы", WIN_REQUIRES_ADMIN, 0, 0, IDR_GRID_POPUP },
  { ID_HISTORICAL_DB_VIEW, "HistoricalDB", L"Базы данных", WIN_REQUIRES_ADMIN, 0, 0, IDR_GRID_POPUP },
  { ID_STATISTICS_VIEW, "Stat", L"Статус", WIN_SING, 300, 400 },
  { ID_SUMMARY_VIEW, "Summ", L"Сводка", WIN_INS | WIN_CAN_PRINT },
  { ID_TIMED_DATA_VIEW, "TimeVal", L"Времена и значения", WIN_INS | WIN_DISALLOW_NEW | WIN_CAN_PRINT, 0, 0, 0 },
  { ID_USERS_VIEW, "Users", L"Пользователи", WIN_REQUIRES_ADMIN, 0, 0, IDR_GRID_POPUP },
  { ID_WEB_VIEW, "Web", L"Web" },
  { ID_PRINT_PREVIEW, "Print", L"Печать", WIN_DISALLOW_NEW },
  { ID_PROPERTY_VIEW, "RecEditor", L"Параметры", WIN_DISALLOW_NEW | WIN_REQUIRES_ADMIN },
  { ID_HARDWARE_VIEW, "Subsystems", L"Оборудование", WIN_SING, 200, 400 },
  { ID_TABLE_EDITOR, "TableEditor", L"Конфигурация", WIN_DISALLOW_NEW | WIN_REQUIRES_ADMIN, 0, 0, IDR_GRID_POPUP },
  { ID_TRANSMISSION_VIEW, "Transmission", L"Ретрансляция", WIN_INS | WIN_DISALLOW_NEW, 0, 0, 0 },
  { ID_EXCEL_REPORT_VIEW, "ExcelReport", L"Отчет", WIN_CAN_PRINT, 0, 0, 0 },
  { ID_VIDICON_DISPLAY_VIEW, "VidiconDisplay", L"Схема", WIN_CAN_PRINT, 0, 0, 0 },
  { ID_NODES_VIEW, "Nodes", L"Узлы", WIN_SING | WIN_REQUIRES_ADMIN, 200, 400, 0 },
};

const WindowInfo* FindWindowInfo(unsigned command_id) {
  static_assert(_countof(g_window_infos) == VIEW_TYPE_COUNT, "Bad windows infos");
  for (int i = 0; i < _countof(g_window_infos); i++)
    if (g_window_infos[i].command_id == command_id)
      return &g_window_infos[i];
  return NULL;
}

const WindowInfo& GetWindowInfo(unsigned command_id) {
  const WindowInfo* info = FindWindowInfo(command_id);
  assert(info);
  return *info;
}

unsigned ParseWindowType(const char* str) {
  for (int i = 0; i < _countof(g_window_infos); i++)
    if (_stricmp(g_window_infos[i].name, str) == 0)
      return g_window_infos[i].command_id;
  return 0;
}

const char* ViewTypeToString(ViewType type) {
  if (type < 0 || type >= VIEW_TYPE_COUNT)
    return "Unknown";
  return g_window_infos[type].name;
}
