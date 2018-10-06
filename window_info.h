#pragma once

#include "base/strings/string16.h"
#include "base/strings/string_piece.h"

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

unsigned ParseWindowType(base::StringPiece str);
const char* ViewTypeToString(unsigned command_id);

extern const WindowInfo g_window_infos[];