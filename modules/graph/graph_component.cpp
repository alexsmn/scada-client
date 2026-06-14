#include "graph/graph_component.h"

#include "aui/translation.h"
#include "controller/action.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_registry.h"
#include "graph/graph_view.h"
#include "resources/common_resources.h"

const WindowInfo kGraphWindowInfo = {
    ID_GRAPH_VIEW, "Graph", u"Graph", WIN_INS, 0, 0, IDR_GRAPH_POPUP};

REGISTER_CONTROLLER(GraphView, kGraphWindowInfo);

void RegisterGraphCommandActions(UiCommandRegistry& ui_command_registry) {
  ui_command_registry.AddAction(Action{.command_id_ = ID_OPEN_GRAPH,
                                       .category_ = CATEGORY_OPEN,
                                       .title_ = Translate("Graph"),
                                       .image_id_ = ID_GRAPH_VIEW,
                                       .flags_ = Action::ALWAYS_VISIBLE});

  ui_command_registry.AddMenuItem({.menu_id = MainMenuId::Graph,
                                   .order = 100,
                                   .command_id = ID_GRAPH_VIEW,
                                   .title = Translate("New")});
}
