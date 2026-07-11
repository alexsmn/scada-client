#include "graph/graph_component.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "base/u16format.h"
#include "controller/action.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_registry.h"
#include "controller/main_menu_window_type_registry.h"
#include "controller/selection_model.h"
#include "core/global_command_context.h"
#include "core/selection_command_context.h"
#include "filesystem/file_cache.h"
#include "graph/graph_view.h"
#include "main_window/main_window_interface.h"
#include "main_window/opened_view/opened_view_interface.h"
#include "modules/selection_command_helpers.h"
#include "node_service/node_ref.h"
#include "profile/window_definition.h"
#include "resources/common_resources.h"
#include "ui/common/client_utils.h"

#include <utility>

namespace {

Awaitable<OpenedViewInterface*> OpenDisplayContainingNode(
    FileCache& file_cache,
    DialogService& dialog_service,
    MainWindowInterface& main_window,
    NodeRef node) {
  auto cached_items =
      file_cache.GetList(ID_MODUS_VIEW).GetFilesContainingItem(node.node_id());

  if (cached_items.empty()) {
    auto msg = u16format(L"Display for item \"{}\" was not found.",
                         ToString16(node.display_name()));
    co_await dialog_service.RunMessageBox(msg, Translate("Display"),
                                          MessageBoxMode::Info);
    co_return nullptr;
  }

  const std::filesystem::path& path = cached_items.front().first;

  OpenedViewInterface* opened_view = nullptr;
  for (OpenedViewInterface* view : main_window.GetOpenedViews()) {
    if (view->Save().path == path) {
      main_window.ActivateView(*view);
      opened_view = view;
      break;
    }
  }

  if (!opened_view) {
    WindowDefinition win(GetWindowInfo(ID_MODUS_VIEW));
    win.path = path;
    opened_view = co_await main_window.OpenView(win);
  }

  opened_view->Select(node.node_id());
  co_return opened_view;
}

}  // namespace

const WindowInfo kGraphWindowInfo = {
    ID_GRAPH_VIEW, "Graph", u"Graph", WIN_INS, 0, 0, IDR_GRAPH_POPUP};

REGISTER_CONTROLLER(GraphView, kGraphWindowInfo);

GraphModule::GraphModule(GraphModuleContext&& context)
    : GraphModuleContext{std::move(context)} {
  RegisterMainMenuFavouritesWindowType(MainMenuId::Graph,
                                       kGraphWindowInfo.name);
  RegisterGraphCommandActions(ui_command_registry_);
  global_commands_.AddCommand(
      BasicCommand<GlobalCommandContext>{ID_OPEN_GRAPH}.set_execute_handler(
          [executor = executor_](const GlobalCommandContext& context) {
            CoSpawn(executor,
                    [&main_window = context.main_window]() -> Awaitable<void> {
                      co_await main_window.OpenView(
                          WindowDefinition{kGraphWindowInfo});
                    });
          }));
  selection_commands_.AddCommand(
      MakeOpenViewSelectionCommand(ID_OPEN_GRAPH, kGraphWindowInfo, executor_));
  selection_commands_.AddCommand(BasicCommand<SelectionCommandContext>{
      .command_id = ID_OPEN_DISPLAY,
      .execute_handler =
          [executor = executor_,
           &file_cache = file_cache_](const SelectionCommandContext& context) {
            CoSpawn(executor, [&file_cache,
                               &dialog_service = context.dialog_service,
                               &main_window = context.main_window,
                               node = context.selection.node()]() mutable {
              return OpenDisplayContainingNode(file_cache, dialog_service,
                                               main_window, std::move(node));
            });
          },
      .available_handler =
          [](const SelectionCommandContext& context) {
            return context.selection.timed_data().connected();
          }});
}

GraphModule::~GraphModule() {
  UnregisterMainMenuFavouritesWindowType(MainMenuId::Graph,
                                         kGraphWindowInfo.name);
}

void RegisterGraphCommandActions(UiCommandRegistry& ui_command_registry) {
  ui_command_registry.AddAction(Action{.command_id_ = ID_OPEN_GRAPH,
                                       .category_ = CATEGORY_OPEN,
                                       .title_ = Translate("Graph"),
                                       .image_id_ = ID_GRAPH_VIEW,
                                       .flags_ = Action::ALWAYS_VISIBLE});
  ui_command_registry.AddAction(Action{.command_id_ = ID_OPEN_DISPLAY,
                                       .category_ = CATEGORY_OPEN,
                                       .title_ = Translate("Display"),
                                       .image_id_ = ID_MODUS_VIEW,
                                       .flags_ = Action::ALWAYS_VISIBLE});
  ui_command_registry.AddAction(Action{.command_id_ = ID_VIEW_LEGEND,
                                       .category_ = CATEGORY_VIEW,
                                       .title_ = Translate("Legend"),
                                       .flags_ = Action::CHECKABLE});
  ui_command_registry.AddAction(Action{.command_id_ = ID_GRAPH_DOTS,
                                       .category_ = CATEGORY_VIEW,
                                       .title_ = Translate("Dots"),
                                       .flags_ = Action::CHECKABLE});
  ui_command_registry.AddAction(Action{.command_id_ = ID_GRAPH_STEPS,
                                       .category_ = CATEGORY_VIEW,
                                       .title_ = Translate("Steps"),
                                       .flags_ = Action::CHECKABLE});
  ui_command_registry.AddAction(Action{.command_id_ = ID_GRAPH_SCROLL_BAR,
                                       .category_ = CATEGORY_VIEW,
                                       .title_ = Translate("Scroll Bar"),
                                       .flags_ = Action::CHECKABLE});
  ui_command_registry.AddAction(Action{.command_id_ = ID_NOW,
                                       .category_ = CATEGORY_VIEW,
                                       .title_ = Translate("Scroll to Now"),
                                       .short_title_ = Translate("Now"),
                                       .flags_ = Action::CHECKABLE});
  if (!ui_command_registry.action_manager().FindAction(ID_GRAPH_COLOR)) {
    ui_command_registry.AddAction(Action{.command_id_ = ID_GRAPH_COLOR,
                                         .category_ = CATEGORY_SETUP,
                                         .title_ = Translate("Line Color..."),
                                         .short_title_ = Translate("Color")});
  }
  ui_command_registry.AddAction(Action{.command_id_ = ID_GRAPH_SETUP,
                                       .category_ = CATEGORY_SETUP,
                                       .title_ = Translate("Graph Setup..."),
                                       .short_title_ = Translate("Setup"),
                                       .image_id_ = ID_GRAPH_VIEW});
  ui_command_registry.AddAction(
      Action{.command_id_ = ID_GRAPH_BK_COLOR,
             .category_ = CATEGORY_SETUP,
             .title_ = Translate("Background Color..."),
             .short_title_ = Translate("Background")});
  ui_command_registry.AddAction(Action{.command_id_ = ID_GRAPH_ADD_PANE,
                                       .category_ = CATEGORY_EDIT,
                                       .title_ = Translate("Add Pane")});
  ui_command_registry.AddAction(Action{.command_id_ = ID_GRAPH_DELETE_PANE,
                                       .category_ = CATEGORY_EDIT,
                                       .title_ = Translate("Delete Pane")});

  ui_command_registry.AddMenuItem({.menu_id = MainMenuId::Graph,
                                   .order = 100,
                                   .command_id = ID_GRAPH_VIEW,
                                   .title = Translate("New")});
}
