#include "modules/node_properties/node_property_component.h"

#include "aui/translation.h"
#include "controller/action.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_registry.h"
#include "controller/selection_model.h"
#include "core/selection_command_context.h"
#include "modules/node_properties/node_property_controller.h"
#include "modules/selection_command_helpers.h"
#include "resources/common_resources.h"

#include <utility>

const WindowInfo kNodePropertyWindowInfo = {
    ID_NEW_PROPERTY_VIEW,
    "NewProps",
    u"Properties",
    WIN_DISALLOW_NEW | WIN_REQUIRES_ADMIN | WIN_SINGLE_ITEM,
    200,
    400};

REGISTER_CONTROLLER(NodePropertyController, kNodePropertyWindowInfo);

NodePropertyModule::NodePropertyModule(NodePropertyModuleContext&& context)
    : NodePropertyModuleContext{std::move(context)} {
  ui_command_registry_.AddAction(Action{.command_id_ = ID_ITEM_PARAMS,
                                        .category_ = CATEGORY_EDIT,
                                        .title_ = Translate("Properties"),
                                        .image_id_ = IDB_RECORD_EDITOR});
  selection_commands_.AddCommand(MakeOpenSingleSelectionCommand(
      ID_ITEM_PARAMS, kNodePropertyWindowInfo, executor_,
      [&session_service =
           session_service_](const SelectionCommandContext& context) {
        return session_service.HasAccessRight(scada::AccessRight::kConfigure) &&
               context.selection.node();
      }));
}
