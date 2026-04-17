#include "main_window/initial_page.h"

#include "resources/common_resources.h"
#include "controller/window_info.h"

Page CreateInitialPage() {
  Page page;

  /*// welcome
  WindowDefinition& def = page.AddWin();
  def.title = _T("Руководство пользователя");
  def.type = WinTypeWeb;*/

  // objects
  WindowDefinition& objects_def =
      page.AddWindow(WindowDefinition(GetWindowInfo(ID_OBJECT_VIEW)));
  objects_def.size = {200, 450};

  // portfolio
  WindowDefinition& portfolio =
      page.AddWindow(WindowDefinition(GetWindowInfo(ID_PORTFOLIO_VIEW)));
  portfolio.size = {200, 450};

  // subsystems
  WindowDefinition& subs_def =
      page.AddWindow(WindowDefinition(GetWindowInfo(ID_HARDWARE_VIEW)));
  subs_def.size = {200, 450};

  // events
  WindowDefinition& events_def =
      page.AddWindow(WindowDefinition(GetWindowInfo(ID_EVENT_VIEW)));
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
