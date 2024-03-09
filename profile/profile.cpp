#include "profile/profile.h"

#include "base/client_paths.h"
#include "base/file_path_util.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/string_piece_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time_utils.h"
#include "base/utils.h"
#include "base/value_util.h"
#include "controller/window_info.h"
#include "events/node_event_provider.h"
#include "model/node_id_util.h"
#include "model/scada_node_ids.h"

#include <ATLComTime.h>

namespace {

Page CreateInitialPage() {
  Page page;

  page.id = 1;
  page.title = u"Лист 1";

  /*// welcome
  WindowDefinition& def = page.AddWin();
  def.title = _T("Руководство пользователя");
  def.type = WinTypeWeb;*/

  // objects
  WindowDefinition& objects_def = page.AddWindow(WindowDefinition(
      /*kObjectTreeWindowInfo*/ GetWindowInfo(ID_OBJECT_VIEW)));
  objects_def.size = {200, 450};

  // portfolio
  WindowDefinition& portfolio = page.AddWindow(WindowDefinition(
      /*kPortfolioWindowInfo*/ GetWindowInfo(ID_PORTFOLIO_VIEW)));
  portfolio.size = {200, 450};

  // subsystems
  WindowDefinition& subs_def = page.AddWindow(WindowDefinition(
      /*kHardwareTreeWindowInfo*/ GetWindowInfo(ID_HARDWARE_VIEW)));
  subs_def.size = {200, 450};

  // events
  WindowDefinition& events_def = page.AddWindow(
      WindowDefinition(/*kEventWindowInfo*/ GetWindowInfo(ID_EVENT_VIEW)));
  events_def.size = {800, 600};
  events_def.visible = false;

#if !defined(UI_WT)
  // Graph with server CPU usage.
  WindowDefinition& graph_def = page.AddWindow(
      WindowDefinition(/*kGraphWindowInfo*/ GetWindowInfo(ID_GRAPH_VIEW)));
  graph_def.size = {800, 300};
  graph_def.AddItem("TimeScale").SetString("span", "0:05:00");
  // TODO: Implement.
  /*for (int i = 1; i <= 2; i++) {
    std::string path = NodeId(NamespaceIndexes::TIT, i).ToString();
    WindowItem& item = graph_def.AddItem("Item");
    item.SetString("path", path);
    item.SetInt("dots", 0);
  }*/
#endif

  // Table with top 10 tss.
  WindowDefinition& table_def = page.AddWindow(
      WindowDefinition(/*kTableWindowInfo*/ GetWindowInfo(ID_TABLE_VIEW)));
  table_def.size = {800, 450};
  // TODO: Implement.
  /*for (int i = 1; i <= 10; i++) {
    std::string path = NodeId(NamespaceIndexes::TS, i).ToString();
    table_def.AddItem("Item").SetString("path", path);
  }*/

  // Layout

  PageLayoutBlock& main = page.layout.main;

  main.split(false);
  main.pos = 20;
  PageLayoutBlock& central_block = main.bottom();
  PageLayoutBlock& left_block = main.top();

  central_block.central = true;
#if !defined(UI_WT)
  central_block.add(graph_def.id);
#endif
  central_block.add(table_def.id);
  central_block.active_window = central_block.wins.front();

  left_block.split(true);
  PageLayoutBlock& left_top_block = left_block.top();
  PageLayoutBlock& left_bottom_block = left_block.bottom();

  left_top_block.wins.push_back(objects_def.id);
  left_top_block.wins.push_back(portfolio.id);
  left_top_block.active_window = objects_def.id;

  left_bottom_block.wins.push_back(subs_def.id);

  return page;
}

void LoadMainWindowDef(MainWindowDef& main_window, const base::Value& data) {
  const int inval = (unsigned)(-1) / 2;
  int left = GetInt(data, "left", inval);
  int top = GetInt(data, "top", inval);
  int width = GetInt(data, "width", inval);
  int height = GetInt(data, "height", inval);
  if (left != inval && top != inval && width != inval && height != inval) {
    main_window.bounds = {left, top, width, height};
  }
  main_window.maximized = GetBool(data, "maximized", false);
  main_window.toolbar = GetBool(data, "toolbar", true);
  main_window.status_bar = GetBool(data, "statusBar", true);
  main_window.page_id = GetInt(data, "page", 0);
}

}  // namespace

// MainWindowDef

MainWindowDef::MainWindowDef() {}

// Profile

Profile::Profile() {}

void Profile::Load(NodeEventProvider& node_event_provider) {
  LOG(INFO) << "Load profile";

  std::string error_message;
  if (auto data = LoadJsonFromFile(GetFilePath(), &error_message)) {
    Load(*data, node_event_provider);
  } else {
    LOG(ERROR) << "Profile load error " << error_message;
  }

  if (pages.empty()) {
    Page page = CreateInitialPage();
    pages[page.id] = page;
  }

  LOG(INFO) << "Profile loaded";
}

void Profile::Load(const base::Value& data,
                   NodeEventProvider& node_event_provider) {
  data_ = data.Clone();

  // common settings
  show_write_ok = GetBool(data, "showWriteOk", show_write_ok);
  event_auto_show = GetBool(data, "showEvents", event_auto_show);
  event_auto_hide = GetBool(data, "hideEvents", event_auto_hide);
  event_flash_window = GetBool(data, "flashOnEvents", event_flash_window);
  event_play_sound = GetBool(data, "soundOnEvents", event_play_sound);

  modus.topology = GetBool(data, "topology", modus.topology);
  modus.modus2 = GetBool(data, "modus2", modus.modus2);

  // TODO: Checked cast.
  scada::EventSeverity severity_min = static_cast<scada::EventSeverity>(
      GetInt(data, "severityMin", static_cast<unsigned>(scada::kSeverityMin)));
  node_event_provider.SetSeverityMin(severity_min);

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
        LOG(ERROR) << "Error " << static_cast<int>(err) << " on load page "
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

void Profile::Save(const NodeEventProvider& node_event_provider) {
  LOG(INFO) << "Save profile";

  for (const auto& writer : writers_) {
    writer(*this);
  }

  auto data = SaveToValue(node_event_provider);

  if (SaveJsonToFile(data, GetFilePath()))
    LOG(INFO) << "Profile saved";
  else
    LOG(ERROR) << "Profile save error";
}

base::Value Profile::SaveToValue(
    const NodeEventProvider& node_event_provider) const {
  base::Value data = data_.Clone();

  // common settings
  SetKey(data, "showWriteOk", show_write_ok);
  SetKey(data, "showEvents", event_auto_show);
  SetKey(data, "hideEvents", event_auto_hide);
  SetKey(data, "flashOnEvents", event_flash_window);
  SetKey(data, "soundOnEvents", event_play_sound);
  SetKey(data, "severityMin",
         static_cast<int>(node_event_provider.severity_min()));

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
      SetKey(wine, "maximized", main_window.maximized);
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
    for (const auto& [id, page] : pages) {
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

Page& Profile::CreatePage() {
  int id = 1;
  while (pages.find(id) != pages.end())
    id++;

  Page& page = pages[id];
  page.id = id;
  page.title = u"Лист " + base::NumberToString16(id);
  return page;
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
