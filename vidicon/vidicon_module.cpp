#include "vidicon/vidicon_module.h"

#include "controller/controller_context.h"
#include "controller/controller_registry.h"
#include "vidicon/display/native/vidicon_display_native_view.h"
#include "vidicon/display/vidicon_display_component.h"
#include "vidicon/teleclient/vidicon_client.h"

VidiconModule::VidiconModule(VidiconModuleContext&& context)
    : VidiconModuleContext{std::move(context)} {
  vidicon_client_ =
      std::make_unique<vidicon::VidiconClient>(vidicon::VidiconClientContext{
          .executor_ = executor_, .timed_data_service_ = timed_data_service_});

  controller_registry_.AddControllerFactory(
      kVidiconDisplayWindowInfo, [this](const ControllerContext& context) {
        return std::make_unique<VidiconDisplayNativeView>(
            VidiconDisplayNativeViewContext{
                .timed_data_service_ = context.timed_data_service_,
                .vidicon_client_ = *vidicon_client_,
                .controller_delegate_ = context.controller_delegate_,
                .dialog_service_ = context.dialog_service_,
                .write_service_ = write_service_});
      });
}

VidiconModule::~VidiconModule() = default;