#include "modules/watch/watch_component.h"

#include "aui/translation.h"
#include "controller/action.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_registry.h"
#include "controller/selection_model.h"
#include "core/selection_command_context.h"
#include "model/devices_node_ids.h"
#include "modules/selection_command_helpers.h"
#include "modules/watch/watch_view.h"
#include "node_service/node_util.h"
#include "resources/common_resources.h"

#include <utility>

const WindowInfo kWatchWindowInfo = {
    ID_WATCH_VIEW, "Log", u"Watch", WIN_DISALLOW_NEW, 0, 0, 0};

REGISTER_CONTROLLER(WatchView, kWatchWindowInfo);

WatchModule::WatchModule(WatchModuleContext&& context)
    : WatchModuleContext{std::move(context)} {
  RegisterWatchCommandActions(ui_command_registry_);
  selection_commands_.AddCommand(MakeOpenSingleSelectionCommand(
      ID_OPEN_WATCH, kWatchWindowInfo, executor_,
      [](const SelectionCommandContext& context) {
        return IsInstanceOf(context.selection.node(),
                            scada::devices::id::DeviceType);
      }));
}

void RegisterWatchCommandActions(UiCommandRegistry& ui_command_registry) {
  ui_command_registry.AddAction(Action{.command_id_ = ID_OPEN_WATCH,
                                       .category_ = CATEGORY_SPECIFIC,
                                       .title_ = Translate("Watch")});
  ui_command_registry.AddAction(Action{.command_id_ = ID_PAUSE,
                                       .category_ = CATEGORY_SPECIFIC,
                                       .title_ = Translate("Pause")});
  ui_command_registry.AddAction(Action{.command_id_ = ID_CLEAR_ALL,
                                       .category_ = CATEGORY_EDIT,
                                       .title_ = Translate("Clear")});
  ui_command_registry.AddAction(Action{.command_id_ = ID_SAVE_AS,
                                       .category_ = CATEGORY_VIEW,
                                       .title_ = Translate("Save As..."),
                                       .short_title_ = Translate("Save")});
}
