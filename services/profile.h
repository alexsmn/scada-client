#pragma once

#include "base/files/file_path.h"
#include "base/time/time.h"
#include "aui/color.h"
#include "aui/rect.h"
#include "scada/node_id.h"
#include "export_util.h"
#include "services/page.h"

#include <map>

class EventFetcher;
class Favourites;
class PortfolioManager;

struct MainWindowDef {
  MainWindowDef();

  int id;
  aui::Rect bounds;
  bool maximized;
  UINT toolbar_position;
  int page_id;
};

class Profile {
 public:
  typedef std::map<int, Page> PageMap;

  Profile();

  Profile(const Profile&) = delete;
  Profile& operator=(const Profile&) = delete;

  Page& CreatePage();

  int CreateWindowId();
  MainWindowDef& GetMainWindow(int id);

  PageMap pages;

  Page trash;

  Page out_wins;  // windows out-of-page

  aui::Color bad_value_color = aui::Rgba{192, 192, 192};
  aui::Color alarm_color = aui::ColorCode::Yellow;

  struct EventJournal {
    base::Value default_state;
  };

  EventJournal event_journal;

  void Load(EventFetcher& event_manager,
            PortfolioManager& portfolio_manager,
            Favourites& favourites);
  void Save(const EventFetcher& event_manager,
            const PortfolioManager& portfolio_manager,
            const Favourites& favourites);

  typedef std::map<int, MainWindowDef> MainWindows;
  MainWindows main_windows;

  // display message box also in case if telecontrol is succeeded
  bool show_write_ok = true;
  // display event window on new events
  bool event_auto_show = true;
  // hide event window when empty
  bool event_auto_hide = true;
  // flash main window if there are any unacknowledged events
  bool event_flash_window = false;
  // play sound if there are any unacknowledged events
  bool event_play_sound = false;

  bool speech_enabled = true;

  bool control_confirmation = true;

  // Use internal Modus rendered instead of ActiveXeme.
  bool modus2 = false;

  struct GraphView {
    base::TimeDelta default_span = base::TimeDelta::FromHours(1);
    aui::Color default_color = aui::ColorCode::White;
    int default_width = 1;
    bool default_scroll_bar = true;
  };

  GraphView graph_view;

  struct TimeRangeDialog {
    int width = 0;
    int height = 0;
  };

  TimeRangeDialog time_range_dialog;

  struct NodeTableController {
    scada::NodeId default_sort_property_id;
  };

  NodeTableController node_table;

  struct TimedData {
    bool mirrored = false;
  };

  TimedData timed_data;

  CsvExportParams csv_export_params;
  std::filesystem::path csv_export_dir;

 private:
  void Load(const base::Value& data,
            EventFetcher& event_manager,
            PortfolioManager& portfolio_manager,
            Favourites& favourites);
  base::Value SaveToValue(const EventFetcher& event_manager,
                          const PortfolioManager& portfolio_manager,
                          const Favourites& favourites) const;

  std::filesystem::path GetFilePath();
};
