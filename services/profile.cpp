#include "services/profile.h"

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/utils.h"
#include "base/xml.h"
#include "client_paths.h"
#include "common/event_manager.h"
#include "common/node_id_util.h"
#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "services/favourites.h"
#include "services/portfolio.h"
#include "services/portfolio_manager.h"
#include "window_info.h"

#include <ATLComTime.h>
#include <fstream>

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

bool ParseTimeDelta(const wchar_t* str, base::TimeDelta& span) {
  int h, m, s;
  if (swscanf(str, L"%d:%d:%d", &h, &m, &s) != 3)
    return false;
  span = base::TimeDelta::FromHours(h) + base::TimeDelta::FromMinutes(m) +
         base::TimeDelta::FromSeconds(s);
  return true;
}

UINT ParseToolbarPosition(const std::wstring& str) {
  if (_wcsicmp(str.c_str(), L"Top") == 0)
    return ID_TOOLBAR_TOP;
  else if (_wcsicmp(str.c_str(), L"Hidden") == 0)
    return ID_TOOLBAR_HIDDEN;
  else
    return ID_TOOLBAR_LEFT;
}

const wchar_t* FormatToolbarPosition(UINT position) {
  switch (position) {
    case ID_TOOLBAR_HIDDEN:
      return L"Hidden";
    case ID_TOOLBAR_TOP:
      return L"Top";
    default:
      return L"Left";
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

void LoadMainWindowDef(MainWindowDef& main_window, xml::Node& node) {
  const int inval = (unsigned)(-1) / 2;
  int left = ParseWithDefault(node.GetAttribute("left"), inval);
  int top = ParseWithDefault(node.GetAttribute("top"), inval);
  int width = ParseWithDefault(node.GetAttribute("width"), inval);
  int height = ParseWithDefault(node.GetAttribute("height"), inval);
  if (left != inval && top != inval && width != inval && height != inval)
    main_window.bounds = gfx::Rect(left, top, width, height);
  main_window.maximized =
      ParseWithDefault(node.GetAttribute("maximized"), false);
  main_window.toolbar_position =
      ParseToolbarPosition(node.GetAttribute("toolbar"));
  main_window.page_id = ParseWithDefault(node.GetAttribute("page"), 0);
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

  try {
    std::ifstream stream(GetFilePath().value().c_str(),
                         std::ios::in | std::ios::binary);
    if (stream.fail())
      throw E_FAIL;
    xml::TextReader reader(stream);
    xml::Document doc;
    doc.Load(reader);

    // root
    xml::Node* doce = doc.GetDocumentElement();
    if (!doce)
      throw E_FAIL;

    // common settings
    xml::Node* paramse = doce->select("Profile");
    if (paramse) {
      show_write_ok =
          ParseWithDefault(paramse->GetAttribute("showWriteOk"), show_write_ok);
      event_auto_show = ParseWithDefault(paramse->GetAttribute("showEvents"),
                                         event_auto_show);
      event_auto_hide = ParseWithDefault(paramse->GetAttribute("hideEvents"),
                                         event_auto_hide);
      event_flash_window = ParseWithDefault(
          paramse->GetAttribute("flashOnEvents"), event_flash_window);
      event_play_sound = ParseWithDefault(
          paramse->GetAttribute("soundOnEvents"), event_play_sound);
      modus2 = ParseWithDefault(paramse->GetAttribute("modus2"), modus2);

      unsigned severity_min =
          ParseWithDefault(paramse->GetAttribute("severityMin"),
                           static_cast<unsigned>(scada::kSeverityMin));
      event_manager.SetSeverityMin(severity_min);
    }

    // window settings
    for (xml::Node* wine = doce->first_child; wine; wine = wine->next) {
      if (_stricmp(wine->name.c_str(), "MainWindow") == 0) {
        MainWindowDef def;
        LoadMainWindowDef(def, *wine);
        if (def.id == 0 || main_windows.find(def.id) != main_windows.end())
          def.id = CreateWindowId();
        main_windows[def.id] = def;
      }
    }

    // pages root
    xml::Node* pagese = doce->select("Pages");
    if (!pagese)
      throw E_FAIL;

    // pages
    for (xml::Node* node = pagese->first_child; node; node = node->next) {
      xml::Node& pagee = *node;
      if (pagee.type != xml::NodeTypeElement)
        continue;
      if (pagee.name.compare("Page") != 0)
        continue;
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

    // out-of-page
    xml::Node* out_pagese = doce->select("OutOfPage");
    if (out_pagese)
      out_wins.Load(*out_pagese);

    // favorites
    xml::Node* favourites_root = doce->select("Favorites");
    if (favourites_root)
      favourites.Load(*favourites_root);

    // portfolios
    xml::Node* pfoliose = doce->select("Portfolios");
    if (pfoliose) {
      auto& portfolios = portfolio_manager.portfolios;
      for (xml::Node* node = pfoliose->first_child; node; node = node->next) {
        xml::Node& pfolioe = *node;
        if (pfolioe.type != xml::NodeTypeElement)
          continue;
        if (pfolioe.name.compare("Portfolio") != 0)
          continue;
        portfolios.push_back(Portfolio());
        Portfolio& portfolio = portfolios.back();
        portfolio.name = pfolioe.GetAttribute("name");
        // items
        for (xml::Node* node = pfolioe.first_child; node; node = node->next) {
          xml::Node& iteme = *node;
          if (iteme.type != xml::NodeTypeElement)
            continue;
          if (iteme.name.compare("Item") != 0)
            continue;

          std::string path =
              base::SysWideToNativeMB(iteme.GetAttribute("path"));
          auto node_id = NodeIdFromScadaString(path);
          if (node_id.is_null()) {
            // TODO: Log.
            continue;
          }

          portfolio.items.insert(node_id);
        }
      }
    }

    // defaults
    if (auto* defse = doce->select("Defaults")) {
      if (auto* graphe = defse->select("Graph")) {
        ParseTimeDelta(graphe->GetAttribute("def_span").c_str(),
                       graph_view.default_span);
        graph_view.default_width =
            ParseWithDefault(graphe->GetAttribute("def_weight"), 1);
      }

      if (auto* node = defse->select("TimeRangeDialog")) {
        time_range_dialog.width =
            ParseWithDefault(node->GetAttribute("width"), 0);
        time_range_dialog.height =
            ParseWithDefault(node->GetAttribute("height"), 0);
      }

      if (auto* node = defse->select("NodeTable")) {
        node_table.default_sort_property_id = NodeIdFromScadaString(
            base::SysWideToNativeMB(node->GetAttribute("sort-property-id")));
      }
    }

    LOG(INFO) << "Profile loaded";

  } catch (HRESULT err) {
    LOG(ERROR) << "Profile load error " << static_cast<int>(err);
  } catch (const xml::Error& /*err*/) {
    LOG(ERROR) << "Profile load XML error";
  }

  if (pages.empty()) {
    Page page = CreateInitialPage();
    pages[page.id] = page;
  }
}

void Profile::Save(const events::EventManager& event_manager,
                   const PortfolioManager& portfolio_manager,
                   const Favourites& favourites) {
  LOG(INFO) << "Save profile";

  try {
    xml::Document doc;

    // xml declaration
    doc.SetVersion(L"1.1");
    doc.SetEncoding(xml::EncodingUtf8);

    // root
    xml::Node& doce = doc.AddElement("Workplace");

    // common settings
    xml::Node& paramse = doce.AddElement("Profile");
    paramse.SetAttribute("showWriteOk", WideFormat(show_write_ok));
    paramse.SetAttribute("showEvents", WideFormat(event_auto_show));
    paramse.SetAttribute("hideEvents", WideFormat(event_auto_hide));
    paramse.SetAttribute("flashOnEvents", WideFormat(event_flash_window));
    paramse.SetAttribute("soundOnEvents", WideFormat(event_play_sound));
    paramse.SetAttribute("severityMin",
                         WideFormat(event_manager.severity_min()));
    paramse.SetAttribute("modus2", WideFormat(modus2));

    // window settings
    for (MainWindows::iterator i = main_windows.begin();
         i != main_windows.end(); ++i) {
      const MainWindowDef& main_window = i->second;
      xml::Node& wine = doce.AddElement("MainWindow");
      wine.SetAttribute("id", WideFormat(main_window.id));
      wine.SetAttribute("left", WideFormat(main_window.bounds.x()));
      wine.SetAttribute("top", WideFormat(main_window.bounds.y()));
      wine.SetAttribute("width", WideFormat(main_window.bounds.width()));
      wine.SetAttribute("height", WideFormat(main_window.bounds.height()));
      wine.SetAttribute("maximized", WideFormat(main_window.maximized));
      wine.SetAttribute("toolbar",
                        FormatToolbarPosition(main_window.toolbar_position));
      wine.SetAttribute("page", WideFormat(main_window.page_id));
    }

    // pages root
    xml::Node& pagese = doce.AddElement("Pages");
    // pages
    for (PageMap::iterator i = pages.begin(); i != pages.end(); i++) {
      Page& page = i->second;
      xml::Node& pagee = pagese.AddElement("Page");
      page.Save(pagee, false);
    }

    // out-of-page
    xml::Node& out_winse = doce.AddElement("OutOfPage");
    out_wins.Save(out_winse, true);

    // favorites
    xml::Node& favourites_root = doce.AddElement("Favorites");
    favourites.Save(favourites_root);

    // portfolios
    xml::Node& pfoliose = doce.AddElement("Portfolios");
    for (auto& portfolio : portfolio_manager.portfolios) {
      xml::Node& pfolioe = pfoliose.AddElement("Portfolio");
      pfolioe.SetAttribute("name", portfolio.name);
      for (std::set<scada::NodeId>::const_iterator j = portfolio.items.begin();
           j != portfolio.items.end(); ++j) {
        const scada::NodeId& node_id = *j;
        xml::Node& iteme = pfolioe.AddElement("Item");
        iteme.SetAttribute("path", NodeIdToScadaString(node_id));
      }
    }

    // defaults
    {
      xml::Node& defse = doce.AddElement("Defaults");

      // GraphView
      {
        xml::Node& graphe = defse.AddElement("Graph");
        graphe.SetAttribute("def_span",
                            FormatTimeDelta(graph_view.default_span));
        graphe.SetAttribute("def_weight", WideFormat(graph_view.default_width));
      }

      // TimeRangeDialog
      {
        xml::Node& node = defse.AddElement("TimeRangeDialog");
        node.SetAttribute("width", WideFormat(time_range_dialog.width));
        node.SetAttribute("height", WideFormat(time_range_dialog.height));
      }

      {
        xml::Node& node = defse.AddElement("NodeTable");
        node.SetAttribute(
            "sort-property-id",
            NodeIdToScadaString(node_table.default_sort_property_id));
      }
    }

    // save
    std::ofstream stream(GetFilePath().value().c_str(),
                         std::ios::out | std::ios::binary);
    xml::TextWriter writer(stream);
    writer.line_breaks = true;
    doc.Save(writer);

    LOG(INFO) << "Profile saved";

  } catch (HRESULT err) {
    LOG(ERROR) << "Profile save error " << static_cast<int>(err);
  } catch (const xml::Error& /*err*/) {
    LOG(ERROR) << "Profile save XML error";
  }
}

base::FilePath Profile::GetFilePath() {
  base::FilePath path;
  base::PathService::Get(client::DIR_PRIVATE, &path);
  return path.Append(L"config.xml");
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

base::Value Profile::ToJson() const {
  return {};
}

void Profile::FromJson(const base::Value& json) {}
