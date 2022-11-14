#include "services/profile.h"

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
#include "client_paths.h"
#include "common/event_fetcher.h"
#include "common_resources.h"
#include "components/events/events_component.h"
#include "components/favourites/favourites.h"
#include "components/hardware_tree/hardware_tree_component.h"
#include "components/object_tree/object_tree_component.h"
#include "components/portfolio/portfolio.h"
#include "components/portfolio/portfolio_component.h"
#include "components/portfolio/portfolio_manager.h"
#include "components/table/table_component.h"
#include "model/node_id_util.h"
#include "model/scada_node_ids.h"
#include "window_info.h"

#if !defined(UI_WT)
#include "components/graph/graph_component.h"
#endif

#include <ATLComTime.h>

namespace {

UINT ParseToolbarPosition(std::string_view str) {
  if (base::EqualsCaseInsensitiveASCII(AsStringPiece(str), "top"))
    return ID_TOOLBAR_TOP;
  else if (base::EqualsCaseInsensitiveASCII(AsStringPiece(str), "hidden"))
    return ID_TOOLBAR_HIDDEN;
  else
    return ID_TOOLBAR_LEFT;
}

std::string_view FormatToolbarPosition(UINT position) {
  switch (position) {
    case ID_TOOLBAR_HIDDEN:
      return "hidden";
    case ID_TOOLBAR_TOP:
      return "top";
    default:
      return "left";
  }
}

Page CreateInitialPage() {
  Page page;

  page.id = 1;
  page.title = u"Лист 1";

  /*// welcome
  WindowDefinition& def = page.AddWin();
  def.title = _T("Руководство пользователя");
  def.type = WinTypeWeb;*/

  // objects
  WindowDefinition& objects_def =
      page.AddWindow(WindowDefinition(kObjectTreeWindowInfo));
  objects_def.size = {200, 450};

  // portfolio
  WindowDefinition& portfolio =
      page.AddWindow(WindowDefinition(kPortfolioWindowInfo));
  portfolio.size = {200, 450};

  // subsystems
  WindowDefinition& subs_def =
      page.AddWindow(WindowDefinition(kHardwareTreeWindowInfo));
  subs_def.size = {200, 450};

  // events
  WindowDefinition& events_def =
      page.AddWindow(WindowDefinition(kEventWindowInfo));
  events_def.size = {800, 600};
  events_def.visible = false;

#if !defined(UI_WT)
  // Graph with server CPU usage.
  WindowDefinition& graph_def =
      page.AddWindow(WindowDefinition(kGraphWindowInfo));
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
  WindowDefinition& table_def =
      page.AddWindow(WindowDefinition(kTableWindowInfo));
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
  if (left != inval && top != inval && width != inval && height != inval)
    main_window.bounds = {left, top, width, height};
  main_window.maximized = GetBool(data, "maximized", false);
  main_window.toolbar_position =
      ParseToolbarPosition(GetString(data, "toolbar"));
  main_window.page_id = GetInt(data, "page", 0);
}

}  // namespace

// MainWindowDef

MainWindowDef::MainWindowDef()
    : id(0), maximized(false), toolbar_position(ID_TOOLBAR_TOP), page_id(0) {}

// Profile

Profile::Profile() {}

void Profile::Load(EventFetcher& event_manager,
                   PortfolioManager& portfolio_manager,
                   Favourites& favourites) {
  LOG(INFO) << "Load profile";

  std::string error_message;
  if (auto data = LoadJsonFromFile(GetFilePath(), &error_message))
    Load(*data, event_manager, portfolio_manager, favourites);
  else
    LOG(ERROR) << "Profile load error " << error_message;

  if (pages.empty()) {
    Page page = CreateInitialPage();
    pages[page.id] = page;
  }

  LOG(INFO) << "Profile loaded";
}

void Profile::Load(const base::Value& data,
                   EventFetcher& event_manager,
                   PortfolioManager& portfolio_manager,
                   Favourites& favourites) {
  // common settings
  show_write_ok = GetBool(data, "showWriteOk", show_write_ok);
  event_auto_show = GetBool(data, "showEvents", event_auto_show);
  event_auto_hide = GetBool(data, "hideEvents", event_auto_hide);
  event_flash_window = GetBool(data, "flashOnEvents", event_flash_window);
  event_play_sound = GetBool(data, "soundOnEvents", event_play_sound);
  modus2 = GetBool(data, "modus2", modus2);

  unsigned severity_min =
      GetInt(data, "severityMin", static_cast<unsigned>(scada::kSeverityMin));
  event_manager.SetSeverityMin(severity_min);

  // window settings
  if (auto* list = GetList(data, "windows")) {
    for (const auto& wine : *list) {
      MainWindowDef def;
      LoadMainWindowDef(def, wine);
      if (def.id == 0 || main_windows.find(def.id) != main_windows.end())
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

  // favorites
  if (auto* favourites_root = data.FindKey("favorites"))
    favourites.Load(*favourites_root);

  // portfolios
  if (auto* pfoliose = GetList(data, "portfolios")) {
    auto& portfolios = portfolio_manager.portfolios;
    for (auto& pfolioe : *pfoliose) {
      Portfolio& portfolio = portfolios.emplace_back();
      portfolio.name = GetString16(pfolioe, "name");
      // items
      if (auto* itemse = GetList(pfolioe, "items")) {
        for (auto& iteme : *itemse) {
          auto path = GetString(iteme, "path");
          auto node_id = NodeIdFromScadaString(path);
          if (!node_id.is_null())
            portfolio.items.insert(node_id);
        }
      }
    }
  }

  if (auto* event_journal = FindDict(data, "eventJournal")) {
    if (auto* state = FindDict(*event_journal, "defaultState"))
      this->event_journal.default_state = state->Clone();
  }

  if (auto* graphe = FindDict(data, "graph")) {
    Deserialize(GetString(*graphe, "def_span"), graph_view.default_span);
    graph_view.default_width = GetInt(*graphe, "def_weight", 1);
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

  if (auto* node = FindDict(data, "csv")) {
    if (auto params = FromJson<CsvExportParams>(*node))
      csv_export_params = std::move(*params);
  }

  csv_export_dir = GetString16(data, "csvPath");
}

void Profile::Save(const EventFetcher& event_manager,
                   const PortfolioManager& portfolio_manager,
                   const Favourites& favourites) {
  LOG(INFO) << "Save profile";

  auto data = SaveToValue(event_manager, portfolio_manager, favourites);

  if (SaveJsonToFile(data, GetFilePath()))
    LOG(INFO) << "Profile saved";
  else
    LOG(ERROR) << "Profile save error";
}

base::Value Profile::SaveToValue(const EventFetcher& event_manager,
                                 const PortfolioManager& portfolio_manager,
                                 const Favourites& favourites) const {
  base::Value data{base::Value::Type::DICTIONARY};

  // common settings
  SetKey(data, "showWriteOk", show_write_ok);
  SetKey(data, "showEvents", event_auto_show);
  SetKey(data, "hideEvents", event_auto_hide);
  SetKey(data, "flashOnEvents", event_flash_window);
  SetKey(data, "soundOnEvents", event_play_sound);
  SetKey(data, "severityMin", static_cast<int>(event_manager.severity_min()));
  SetKey(data, "modus2", modus2);

  // window settings
  {
    base::Value::ListStorage list;
    for (const auto& [id, main_window] : main_windows) {
      base::Value wine{base::Value::Type::DICTIONARY};
      SetKey(wine, "id", main_window.id);
      SetKey(wine, "left", main_window.bounds.x);
      SetKey(wine, "top", main_window.bounds.y);
      SetKey(wine, "width", main_window.bounds.width);
      SetKey(wine, "height", main_window.bounds.height);
      SetKey(wine, "maximized", main_window.maximized);
      SetKey(wine, "toolbar",
             FormatToolbarPosition(main_window.toolbar_position));
      SetKey(wine, "page", main_window.page_id);
      list.emplace_back(std::move(wine));
    }
    data.SetKey("windows", base::Value{std::move(list)});
  }

  // pages root
  {
    base::Value::ListStorage list;
    for (const auto& [id, page] : pages)
      list.emplace_back(page.Save(false));
    data.SetKey("pages", base::Value{std::move(list)});
  }

  // out-of-page
  data.SetKey("floatingWindows", out_wins.Save(true));

  // favorites
  data.SetKey("favorites", favourites.Save());

  // portfolios
  {
    base::Value::ListStorage portfolio_storage;
    for (auto& portfolio : portfolio_manager.portfolios) {
      base::Value pfolioe{base::Value::Type::DICTIONARY};
      SetKey(pfolioe, "name", portfolio.name);
      {
        base::Value::ListStorage item_storage;
        for (const auto& node_id : portfolio.items)
          item_storage.emplace_back(NodeIdToScadaString(node_id));
        pfolioe.SetKey("items", base::Value{std::move(item_storage)});
      }
      portfolio_storage.emplace_back(std::move(pfolioe));
    }
    data.SetKey("portfolios", base::Value{std::move(portfolio_storage)});
  }

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

  data.SetKey("csv", ToJson(csv_export_params));
  SetKey(data, "csvPath", csv_export_dir.u16string());

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
  int id = 1;
  while (main_windows.find(id) != main_windows.end())
    ++id;
  return id;
}

MainWindowDef& Profile::GetMainWindow(int id) {
  MainWindowDef& window = main_windows[id];
  window.id = id;
  return window;
}
