#pragma once

#include "base/files/file_util.h"
#include "base/time/time.h"
#include "core/SkColor.h"
#include "ui/gfx/rect.h"
#include "services/page.h"

#include <map>

namespace events {
class EventManager;
}

class Favourites;
class PortfolioManager;

struct MainWindowDef {
  MainWindowDef();

  int id;
  gfx::Rect bounds;
  bool maximized;
  UINT toolbar_position;
  unsigned page_id;
};

class Profile {
 public:
  typedef std::map<int, Page>	PageMap;

  Profile();

  Page& CreatePage();

  int CreateWindowId();
  MainWindowDef& GetMainWindow(int id);

  PageMap& pages() { return pages_; }
  Page& trash() { return trash_; }
  Page& out_wins() { return out_wins_; }
  SkColor bad_value_color() const { return bad_value_color_; }

  struct Column {
    int id;
    int width;
  };
  typedef std::vector<Column> Columns;
  void set_default_table_columns(const Columns& columns) { columns_ = columns; }

  void set_bad_value_color(SkColor color) { bad_value_color_ = color; }

  void Load(PortfolioManager& portfolio_manager, events::EventManager& event_manager, Favourites& favourites);
  void Save(const PortfolioManager& portfolio_manager, const events::EventManager& event_manager, const Favourites& favourites);

  typedef std::map<int, MainWindowDef> MainWindows;
  MainWindows main_windows;

  // display message box also in case if telecontrol is succeeded
  bool show_write_ok;
  // display event window on new events
  bool event_auto_show;
  // hide event window when empty
  bool event_auto_hide;
  // flash main window if there are any unacknowledged events
  bool event_flash_window;
  // play sound if there are any unacknowledged events
  bool event_play_sound;

  bool speech_enabled;

  bool control_confirmation;

  // Use internal Modus rendered instead of ActiveXeme.
  bool modus2;

  // graph settings
  base::TimeDelta default_graph_span;
  SkColor graph_def_color;
  int graph_def_width;

private:
  base::FilePath GetFilePath();

  PageMap		pages_;
  Page		trash_;
  Page		out_wins_;			// windows out-of-page
  SkColor bad_value_color_;
  Columns columns_;

  DISALLOW_COPY_AND_ASSIGN(Profile);
};
