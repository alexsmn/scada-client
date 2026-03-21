#include "profile/profile.h"

#include "aui/translation.h"
#include <boost/log/trivial.hpp>
#include "base/client_paths.h"
#include "base/file_path_util.h"
#include "base/path_service.h"
#include "base/format.h"
#include "base/time_utils.h"
#include "base/utils.h"
#include "base/value_util.h"
#include "model/node_id_util.h"
#include "model/scada_node_ids.h"

#include <ATLComTime.h>

namespace {

void LoadMainWindowDef(MainWindowDef& main_window, const base::Value& data) {
  const int inval = (unsigned)(-1) / 2;
  int left = GetInt(data, "left", inval);
  int top = GetInt(data, "top", inval);
  int width = GetInt(data, "width", inval);
  int height = GetInt(data, "height", inval);
  if (left != inval && top != inval && width != inval && height != inval) {
    main_window.bounds = {left, top, width, height};
  }
  if (GetBool(data, "maximized", false)) {
    main_window.state = MainWindowDef::State::kMaximized;
  }
  main_window.toolbar = GetBool(data, "toolbar", true);
  main_window.status_bar = GetBool(data, "statusBar", true);
  main_window.page_id = GetInt(data, "page", 0);
}

}  // namespace

// MainWindowDef

MainWindowDef::MainWindowDef() {}

// Profile

Profile::Profile() {}

void Profile::Load() {
  BOOST_LOG_TRIVIAL(info) << "Load profile";

  std::string error_message;
  if (auto data = LoadJsonFromFile(GetFilePath(), &error_message)) {
    Load(*data);
  } else {
    BOOST_LOG_TRIVIAL(error) << "Profile load error " << error_message;
  }

  BOOST_LOG_TRIVIAL(info) << "Profile loaded";
}

void Profile::Load(const base::Value& data) {
  data_ = data.Clone();

  // common settings
  show_write_ok = GetBool(data, "showWriteOk", show_write_ok);
  event_auto_show = GetBool(data, "showEvents", event_auto_show);
  event_auto_hide = GetBool(data, "hideEvents", event_auto_hide);
  event_flash_window = GetBool(data, "flashOnEvents", event_flash_window);
  event_play_sound = GetBool(data, "soundOnEvents", event_play_sound);

  modus.topology = GetBool(data, "topology", modus.topology);
  modus.modus2 = GetBool(data, "modus2", modus.modus2);

  // window settings
  if (auto* list = GetList(data, "windows")) {
    for (const auto& wine : *list) {
      MainWindowDef def;
      LoadMainWindowDef(def, wine);
      if (def.id == 0 || main_windows.contains(def.id))
        def.id = CreateWindowId();
      main_windows[def.id] = def;
    }
  }

  // pages root
  if (auto* pagese = GetList(data, "pages")) {
    for (auto& pagee : *pagese) {
      Page page;
      try {
        page.Load(pagee);
      } catch (HRESULT err) {
        BOOST_LOG_TRIVIAL(error) << "Error " << static_cast<int>(err) << " on load page "
                   << page.id;
        page.id = 0;
      }
      if (page.id)
        pages[page.id] = page;
    }
  }

  // out-of-page
  if (auto* out_pagese = data.FindKey("floatingWindows"))
    out_wins.Load(*out_pagese);

  if (auto* event_journal = FindDict(data, "eventJournal")) {
    if (auto* state = FindDict(*event_journal, "defaultState"))
      this->event_journal.default_state = state->Clone();
  }

  if (auto* graphe = FindDict(data, "graph")) {
    Deserialize(GetString(*graphe, "def_span"), graph_view.default_span);
    graph_view.default_width = GetInt(*graphe, "def_weight", 1);
    graph_view.default_scroll_bar = GetBool(*graphe, "def_scroll_bar", 1);
  }

  if (auto* node = FindDict(data, "timeRangeDialog")) {
    time_range_dialog.width = GetInt(*node, "width", 0);
    time_range_dialog.height = GetInt(*node, "height", 0);
  }

  if (auto* node = FindDict(data, "nodeTable")) {
    node_table.default_sort_property_id =
        NodeIdFromScadaString(GetString(*node, "sort-property-id"));
  }

  if (auto* node = FindDict(data, "timedData"))
    timed_data.mirrored = GetBool(*node, "mirrored");
}

void Profile::Save() {
  BOOST_LOG_TRIVIAL(info) << "Save profile";

  for (const Writer& writer : writers_) {
    writer(*this);
  }

  auto data = SaveToValue();

  if (SaveJsonToFile(data, GetFilePath()))
    BOOST_LOG_TRIVIAL(info) << "Profile saved";
  else
    BOOST_LOG_TRIVIAL(error) << "Profile save error";
}

base::Value Profile::SaveToValue() const {
  base::Value data = data_.Clone();

  // common settings
  SetKey(data, "showWriteOk", show_write_ok);
  SetKey(data, "showEvents", event_auto_show);
  SetKey(data, "hideEvents", event_auto_hide);
  SetKey(data, "flashOnEvents", event_flash_window);
  SetKey(data, "soundOnEvents", event_play_sound);

  SetKey(data, "topology", modus.topology);
  SetKey(data, "modus2", modus.modus2);

  // window settings
  {
    base::Value::ListStorage list;
    for (const auto& [id, main_window] : main_windows) {
      base::Value wine{base::Value::Type::DICTIONARY};
      SetKey(wine, "id", main_window.id);
      SetKey(wine, "left", main_window.bounds.x());
      SetKey(wine, "top", main_window.bounds.y());
      SetKey(wine, "width", main_window.bounds.width());
      SetKey(wine, "height", main_window.bounds.height());
      if (main_window.state == MainWindowDef::State::kMaximized) {
        SetKey(wine, "maximized", true);
      }
      SetKey(wine, "toolbar", main_window.toolbar);
      SetKey(wine, "statusBar", main_window.status_bar);
      SetKey(wine, "page", main_window.page_id);
      list.emplace_back(std::move(wine));
    }
    data.SetKey("windows", base::Value{std::move(list)});
  }

  // pages root
  {
    base::Value::ListStorage list;
    for (const auto& [_, page] : pages) {
      list.emplace_back(page.Save(false));
    }
    data.SetKey("pages", base::Value{std::move(list)});
  }

  // out-of-page
  data.SetKey("floatingWindows", out_wins.Save(true));

  // Event Journal
  {
    base::Value value{base::Value::Type::DICTIONARY};
    value.SetKey("defaultState", event_journal.default_state.Clone());
    data.SetKey("eventJournal", std::move(value));
  }

  // GraphView
  {
    base::Value graphe{base::Value::Type::DICTIONARY};
    SetKey(graphe, "def_span", SerializeToString(graph_view.default_span));
    SetKey(graphe, "def_weight", graph_view.default_width);
    SetKey(graphe, "def_scroll_bar", graph_view.default_scroll_bar);
    data.SetKey("graph", std::move(graphe));
  }

  // TimeRangeDialog
  {
    base::Value node{base::Value::Type::DICTIONARY};
    SetKey(node, "width", time_range_dialog.width);
    SetKey(node, "height", time_range_dialog.height);
    data.SetKey("timeRangeDialog", std::move(node));
  }

  // NodeTable
  {
    base::Value node{base::Value::Type::DICTIONARY};
    if (!node_table.default_sort_property_id.is_null()) {
      SetKey(node, "sort-property-id",
             NodeIdToScadaString(node_table.default_sort_property_id));
    }
    data.SetKey("nodeTable", std::move(node));
  }

  // TimedData
  {
    base::Value node{base::Value::Type::DICTIONARY};
    SetKey(node, "mirrored", timed_data.mirrored);
    data.SetKey("timedData", std::move(node));
  }

  for (const auto& serializer : serializers_) {
    serializer(data);
  }

  return data;
}

std::filesystem::path Profile::GetFilePath() {
  base::FilePath path;
  base::PathService::Get(client::DIR_PRIVATE, &path);
  return AsFilesystemPath(path.Append(L"profile.json"));
}

Page& Profile::AddPage(const Page& page) {
  int page_id = 1;
  while (pages.contains(page_id)) {
    page_id++;
  }

  auto [iter, ok] = pages.try_emplace(page_id, page);

  Page& new_page = iter->second;
  new_page.id = page_id;

  if (new_page.title.empty()) {
    new_page.title = Translate("Page ") + WideFormat(page_id);
  }

  return new_page;
}

int Profile::CreateWindowId() {
  int new_id = 1;
  while (main_windows.contains(new_id)) {
    ++new_id;
  }
  return new_id;
}

MainWindowDef& Profile::GetMainWindow(int main_window_id) {
  MainWindowDef& window = main_windows[main_window_id];
  window.id = main_window_id;
  return window;
}

MainWindowDef* Profile::FindMainWindow(int main_window_id) {
  auto i = main_windows.find(main_window_id);
  return i != main_windows.end() ? &i->second : nullptr;
}
