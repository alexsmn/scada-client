#include "services/profile.h"

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/utils.h"
#include "client_paths.h"
#include "common/event_manager.h"
#include "common/node_id_util.h"
#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "services/favourites.h"
#include "services/portfolio.h"
#include "services/portfolio_manager.h"
#include "value_util.h"
#include "window_info.h"

#include <ATLComTime.h>

namespace {

std::wstring FormatTimeDelta(base::TimeDelta span) {
  __int64 reminder = span.InSeconds();
  int seconds = reminder % 60;
  reminder /= 60;
  int minutes = reminder % 60;
  reminder /= 60;
  int hours = static_cast<int>(reminder);

  return base::StringPrintf(L"%d:%02d:%02d", hours, minutes, seconds);
}

bool ParseTimeDelta(base::StringPiece str, base::TimeDelta& span) {
  int h, m, s;
  if (sscanf(str.as_string().c_str(), "%d:%d:%d", &h, &m, &s) != 3)
    return false;
  span = base::TimeDelta::FromHours(h) + base::TimeDelta::FromMinutes(m) +
         base::TimeDelta::FromSeconds(s);
  return true;
}

UINT ParseToolbarPosition(base::StringPiece str) {
  if (base::EqualsCaseInsensitiveASCII(str, "top"))
    return ID_TOOLBAR_TOP;
  else if (base::EqualsCaseInsensitiveASCII(str, "hidden"))
    return ID_TOOLBAR_HIDDEN;
  else
    return ID_TOOLBAR_LEFT;
}

base::StringPiece FormatToolbarPosition(UINT position) {
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
  page.title = L"Лист 1";

  /*// welcome
  WindowDefinition& def = page.AddWin();
  def.title = _T("Руководство пользователя");
  def.type = WinTypeWeb;*/

  // objects
  WindowDefinition& objects_def =
      page.AddWindow(WindowDefinition(GetWindowInfo(ID_OBJECT_VIEW)));
  objects_def.size = gfx::Size(200, 450);

  // portfolio
  WindowDefinition& portfolio =
      page.AddWindow(WindowDefinition(GetWindowInfo(ID_PORTFOLIO_VIEW)));
  portfolio.size = gfx::Size(200, 450);

  // subsystems
  WindowDefinition& subs_def =
      page.AddWindow(WindowDefinition(GetWindowInfo(ID_HARDWARE_VIEW)));
  subs_def.size = gfx::Size(200, 450);

  // events
  WindowDefinition& events_def =
      page.AddWindow(WindowDefinition(GetWindowInfo(ID_EVENT_VIEW)));
  events_def.size = gfx::Size(800, 600);
  events_def.visible = false;

  // Graph with server CPU usage.
  WindowDefinition& graph_def =
      page.AddWindow(WindowDefinition(GetWindowInfo(ID_GRAPH_VIEW)));
  graph_def.size = gfx::Size(800, 300);
  graph_def.AddItem("TimeScale").SetString("span", "0:05:00");
  // TODO: Implement.
  /*for (int i = 1; i <= 2; i++) {
    std::string path = NodeId(NamespaceIndexes::TIT, i).ToString();
    WindowItem& item = graph_def.AddItem("Item");
    item.SetString("path", path);
    item.SetInt("dots", 0);
  }*/

  // Table with top 10 tss.
  WindowDefinition& table_def =
      page.AddWindow(WindowDefinition(GetWindowInfo(ID_TABLE_VIEW)));
  table_def.size = gfx::Size(800, 450);
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
  central_block.add(graph_def.id);
  central_block.add(table_def.id);
  central_block.active_window = graph_def.id;

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
    main_window.bounds = gfx::Rect(left, top, width, height);
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

void Profile::Load(events::EventManager& event_manager,
                   PortfolioManager& portfolio_manager,
                   Favourites& favourites) {
  LOG(INFO) << "Load profile";

  std::string error_message;
  if (auto data = LoadJson(GetFilePath(), &error_message))
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
                   events::EventManager& event_manager,
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
  if (auto* out_pagese = GetDict(data, "floatingWindows"))
    out_wins.Load(*out_pagese);

  // favorites
  if (auto* favourites_root = GetDict(data, "favorites"))
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

  if (auto* graphe = GetDict(data, "graph")) {
    ParseTimeDelta(GetString(*graphe, "def_span"), graph_view.default_span);
    graph_view.default_width = GetInt(*graphe, "def_weight", 1);
  }

  if (auto* node = GetDict(data, "timeRangeDialog")) {
    time_range_dialog.width = GetInt(*node, "width", 0);
    time_range_dialog.height = GetInt(*node, "height", 0);
  }

  if (auto* node = GetDict(data, "nodeTable")) {
    node_table.default_sort_property_id =
        NodeIdFromScadaString(GetString(*node, "sort-property-id"));
  }
}

void Profile::Save(const events::EventManager& event_manager,
                   const PortfolioManager& portfolio_manager,
                   const Favourites& favourites) {
  LOG(INFO) << "Save profile";

  auto data = SaveToValue(event_manager, portfolio_manager, favourites);

  if (SaveJson(data, GetFilePath()))
    LOG(INFO) << "Profile saved";
  else
    LOG(ERROR) << "Profile save error";
}

base::Value Profile::SaveToValue(const events::EventManager& event_manager,
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
      SetKey(wine, "left", main_window.bounds.x());
      SetKey(wine, "top", main_window.bounds.y());
      SetKey(wine, "width", main_window.bounds.width());
      SetKey(wine, "height", main_window.bounds.height());
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
  data.SetKey("favourites", favourites.Save());

  // portfolios
  {
    base::Value::ListStorage list;
    for (auto& portfolio : portfolio_manager.portfolios) {
      base::Value pfolioe{base::Value::Type::DICTIONARY};
      SetKey(pfolioe, "name", portfolio.name);
      {
        base::Value::ListStorage list;
        for (const auto& node_id : portfolio.items)
          list.emplace_back(NodeIdToScadaString(node_id));
        pfolioe.SetKey("items", base::Value{std::move(list)});
      }
      list.emplace_back(std::move(pfolioe));
    }
    data.SetKey("portfolios", base::Value{std::move(list)});
  }

  // GraphView
  {
    base::Value graphe{base::Value::Type::DICTIONARY};
    SetKey(graphe, "def_span", FormatTimeDelta(graph_view.default_span));
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

  {
    base::Value node{base::Value::Type::DICTIONARY};
    if (!node_table.default_sort_property_id.is_null()) {
      SetKey(node, "sort-property-id",
             NodeIdToScadaString(node_table.default_sort_property_id));
    }
    data.SetKey("nodeTable", std::move(node));
  }

  return data;
}

base::FilePath Profile::GetFilePath() {
  base::FilePath path;
  base::PathService::Get(client::DIR_PRIVATE, &path);
  return path.Append(L"profile.json");
}

Page& Profile::CreatePage() {
  int id = 1;
  while (pages.find(id) != pages.end())
    id++;

  Page& page = pages[id];
  page.id = id;
  page.title = base::WideToUTF16(L"Лист ") + base::NumberToString16(id);
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
