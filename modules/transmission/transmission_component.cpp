#include "modules/transmission/transmission_component.h"

#include "aui/translation.h"
#include "controller/action.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_registry.h"
#include "controller/selection_model.h"
#include "core/selection_command_context.h"
#include "model/devices_node_ids.h"
#include "modules/selection_command_helpers.h"
#include "modules/transmission/transmission_view.h"
#include "node_service/node_util.h"
#include "resources/common_resources.h"

#include <utility>

const WindowInfo kTransmissionWindowInfo = {ID_TRANSMISSION_VIEW,
                                            "Transmission",
                                            u"Transmission",
                                            WIN_INS | WIN_DISALLOW_NEW,
                                            0,
                                            0,
                                            0};

REGISTER_CONTROLLER(TransmissionView, kTransmissionWindowInfo);

TransmissionModule::TransmissionModule(TransmissionModuleContext&& context)
    : TransmissionModuleContext{std::move(context)} {
  ui_command_registry_.AddAction(
      Action{.command_id_ = ID_TRANSMISSION_VIEW,
             .category_ = CATEGORY_EDIT,
             .title_ = Translate("Transmission Table"),
             .short_title_ = Translate("Transmission")});
  selection_commands_.AddCommand(MakeOpenSingleSelectionCommand(
      ID_TRANSMISSION_VIEW, kTransmissionWindowInfo, executor_,
      [&session_service =
           session_service_](const SelectionCommandContext& context) {
        return session_service.HasPrivilege(scada::Privilege::Configure) &&
               IsInstanceOf(context.selection.node(),
                            devices::id::DeviceType) &&
               !IsInstanceOf(context.selection.node(), devices::id::LinkType);
      }));
}
