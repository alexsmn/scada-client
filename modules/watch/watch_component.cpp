#include "modules/watch/watch_component.h"

#include "aui/translation.h"
#include "controller/action.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_registry.h"
#include "modules/watch/watch_view.h"
#include "resources/common_resources.h"

#include <utility>

const WindowInfo kWatchWindowInfo = {
    ID_WATCH_VIEW, "Log", u"Watch", WIN_DISALLOW_NEW, 0, 0, 0};

REGISTER_CONTROLLER(WatchView, kWatchWindowInfo);

WatchModule::WatchModule(WatchModuleContext&& context)
    : WatchModuleContext{std::move(context)} {
  RegisterWatchCommandActions(ui_command_registry_);
}

void RegisterWatchCommandActions(UiCommandRegistry& ui_command_registry) {
  ui_command_registry.AddAction(Action{.command_id_ = ID_PAUSE,
                                       .category_ = CATEGORY_SPECIFIC,
                                       .title_ = Translate("Pause")});
  ui_command_registry.AddAction(Action{.command_id_ = ID_CLEAR_ALL,
                                       .category_ = CATEGORY_EDIT,
                                       .title_ = Translate("Clear")});
}
