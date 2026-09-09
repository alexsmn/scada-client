#pragma once

#include "aui/color.h"
#include "aui/rect.h"
#include "base/lifetime.h"
#include "base/time/time.h"
#include "profile/page.h"
#include "resources/common_resources.h"
#include "scada/date_time.h"
#include "scada/node_id.h"

#include <boost/json.hpp>
#include <boost/signals2/signal.hpp>
#include <map>

struct MainWindowDef {
  MainWindowDef();

  enum class State { kNormal, kMaximized, kMinimized };

  int id = 0;
  scada::aui::Rect bounds;
  State state = State::kNormal;
  int page_id = 0;
  bool toolbar = false;
  bool status_bar = true;
  // The activity rail's selected left-pane mode, as a `PaneMode::key`. Empty
  // means "never chosen" — the window infers one from the page it opens.
  // Stored as a string, not an enum, so a value written by another build
  // degrades to the default instead of selecting the wrong mode.
  std::string pane_mode;
};

class Profile {
 public:
  using PageMap = std::map<int, Page>;

  Profile();

  Profile(const Profile&) = delete;
  Profile& operator=(const Profile&) = delete;

  Page& AddPage(const Page& page);

  int CreateWindowId();
  MainWindowDef& GetMainWindow(int main_window_id);
  MainWindowDef* FindMainWindow(int main_window_id);

  PageMap pages;

  Page trash;

  Page out_wins;  // windows out-of-page

  scada::aui::Color bad_value_color = scada::aui::Rgba{192, 192, 192};
  scada::aui::Color alarm_color = scada::aui::ColorCode::Yellow;

  struct EventJournal {
    boost::json::value default_state;
  };

  EventJournal event_journal;

  void Load();
  // Writes the profile to its file. Never throws: it runs from
  // ~ClientApplication, where an escaping exception is std::terminate.
  void Save();

  // Loads from an already-parsed profile document. A root that is not a JSON
  // object is rejected and the profile keeps its current state.
  void Load(const boost::json::value& data);

  // Serializes the current profile after running registered writers.
  boost::json::value SaveToValue();

  using MainWindows = std::map<int, MainWindowDef>;
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

    // Use the internal Modus renderer instead of ActiveX.
    bool modus2 = false;
  };

  Modus modus;

  struct GraphView {
    scada::Duration default_span = std::chrono::hours(1);
    scada::aui::Color default_color = scada::aui::ColorCode::White;
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

  const boost::json::value& data() const SCADA_LIFETIME_BOUND { return data_; }
  boost::json::value& data() SCADA_LIFETIME_BOUND { return data_; }

  using Writer = std::function<void(Profile& profile)>;

  void RegisterWriter(const Writer& writer) { writers_.emplace_back(writer); }

  using Serializer = std::function<void(boost::json::value& data)>;

  void RegisterSerializer(const Serializer& serializer) {
    serializers_.emplace_back(serializer);
  }

  template <class O>
  [[nodiscard]] boost::signals2::scoped_connection AddChangeObserver(
      O&& observer) {
    return profile_change_signal_.connect(std::forward<O>(observer));
  }

  void NotifyChange() { profile_change_signal_(); }

 private:
  boost::json::value SerializeToValue() const;

  std::filesystem::path GetFilePath();

  boost::json::value data_{boost::json::object{}};

  std::vector<Writer> writers_;
  std::vector<Serializer> serializers_;

  boost::signals2::signal<void()> profile_change_signal_;
};
