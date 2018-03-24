#pragma once

#include "base/strings/string16.h"

enum ViewType {
  VIEW_TYPE_NEW_PROPERTIES,
  VIEW_TYPE_CELLS,
  VIEW_TYPE_SHEET,
  VIEW_TYPE_EVENTS,
  VIEW_TYPE_EVENT_JOURNAL,
  VIEW_TYPE_FAVOURITES,
  VIEW_TYPE_TS_FORMATS,
  VIEW_TYPE_GRAPH,
  VIEW_TYPE_WATCH,
  VIEW_TYPE_MODUS,
  VIEW_TYPE_PORTFOLIO,
  VIEW_TYPE_OBJECTS,
  VIEW_TYPE_TABLE,
  VIEW_TYPE_SIMULATION_ITEMS,
  VIEW_TYPE_HISTORICAL_DB,
  VIEW_TYPE_STATUS,
  VIEW_TYPE_SUMMARY,
  VIEW_TYPE_TIMED_DATA,
  VIEW_TYPE_USERS,
  VIEW_TYPE_WEB,
  VIEW_TYPE_PRINT_PREVIEW,
  VIEW_TYPE_RECORD_EDITOR,
  VIEW_TYPE_DEVICES,
  VIEW_TYPE_TABLE_RECORD_EDITOR,
  VIEW_TYPE_TRANSMISSION,
  VIEW_TYPE_EXCEL_REPORT,
  VIEW_TYPE_VIDICON_DISPLAY,
  VIEW_TYPE_TYPES,
  VIEW_TYPE_NODES,
  VIEW_TYPE_COUNT
};

enum WindowFlags {
  WIN_SING            = 0x0001,	// only single window allowed for page
  WIN_INS             = 0x0002,	// items can be inserted into this window
  WIN_DOCKB	          = 0x0004,	// dock window bottom (only with WIN_SING)
  WIN_DISALLOW_NEW    = 0x0008, // allow to create new window
  WIN_CAN_PRINT       = 0x0010, // can print contents
  WIN_REQUIRES_ADMIN  = 0x0020, // user must have admin rights to create window
};

struct WindowInfo {
  unsigned command_id;
  const char* name;
  const base::char16* title;
  unsigned flags;
  unsigned cx;
  unsigned cy;
  unsigned menu;

  bool is_pane() const { return (flags & WIN_SING) != 0; }
  bool dock_bottom() const { return (flags & WIN_DOCKB) != 0; }
  bool can_insert_item() const { return (flags & WIN_INS) != 0; }
  bool printable() const { return (flags & WIN_CAN_PRINT) != 0; }
  bool createable() const { return !(flags & WIN_DISALLOW_NEW); }
  bool requires_admin_rights() const { return (flags & WIN_REQUIRES_ADMIN) != 0; }
};

const WindowInfo* FindWindowInfo(unsigned command_id);
const WindowInfo& GetWindowInfo(unsigned command_id);

unsigned ParseWindowType(const char* str);

const char* ViewTypeToString(ViewType type);

extern const WindowInfo g_window_infos[];