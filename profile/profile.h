#pragma once

#include "aui/color.h"
#include "aui/rect.h"
#include "base/files/file_path.h"
#include "base/time/time.h"
#include "common_resources.h"
#include "profile/page.h"
#include "scada/node_id.h"

#include <boost/signals2/signal.hpp>
#include <map>

class NodeEventProvider;

struct MainWindowDef {
  MainWindowDef();

  int id = 0;
  aui::Rect bounds;
  bool maximized = false;
  UINT toolbar_position = ID_TOOLBAR_TOP;
  int page_id = 0;
};

class Profile {
 public:
  using PageMap = std::map<int, Page>;

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

  // TODO: Remove the dependencies.
  void Load(NodeEventProvider& node_event_provider);
  void Save(const NodeEventProvider& node_event_provider);

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

  struct Modus {
    bool topology = true;

    // Use internal Modus rendered instead of ActiveXeme.
    bool modus2 = false;
  };

  Modus modus;

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

  const base::Value& data() const { return data_; }
  base::Value& data() { return data_; }

  using Writer = std::function<void(Profile& profile)>;

  void RegisterWriter(const Writer& writer) { writers_.emplace_back(writer); }

  using Serializer = std::function<void(base::Value& data)>;

  void RegisterSerializer(const Serializer& serializer) {
    serializers_.emplace_back(serializer);
  }

  [[nodiscard]] boost::signals2::scoped_connection AddChangeObserver(
      std::function<void()> observer) {
    return profile_change_signal_.connect(std::move(observer));
  }

  void NotifyChange() { profile_change_signal_(); }

 private:
  void Load(const base::Value& data, NodeEventProvider& node_event_provider);

  base::Value SaveToValue(const NodeEventProvider& node_event_provider) const;

  std::filesystem::path GetFilePath();

  base::Value data_;

  std::vector<Writer> writers_;
  std::vector<Serializer> serializers_;

  boost::signals2::signal<void()> profile_change_signal_;
};
