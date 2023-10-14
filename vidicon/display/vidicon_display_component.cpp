#include "vidicon/display/vidicon_display_component.h"

#include "controller/controller_context.h"
#include "controller/controller_registry.h"
#include "vidicon/display/activex/vidicon_display_activex_view.h"
#include "vidicon/display/native/vidicon_display_native_view.h"

const WindowInfo kVidiconDisplayWindowInfo = {
    ID_VIDICON_DISPLAY_VIEW, "VidiconDisplay", u"Схема", 0, 0, 0, 0};

class VidiconDisplayViewFactory : public ControllerRegistrarBase {
 public:
  VidiconDisplayViewFactory()
      : ControllerRegistrarBase{kVidiconDisplayWindowInfo} {}

  virtual std::unique_ptr<Controller> CreateController(
      const ControllerContext& context) override {
    // return
    // std::make_unique<VidiconDisplayActiveXView>(context.vidicon_client_);
    return std::make_unique<VidiconDisplayNativeView>(
        VidiconDisplayNativeViewContext{
            .timed_data_service_ = context.timed_data_service_,
            .vidicon_client_ = context.vidicon_client_,
            .controller_delegate_ = context.controller_delegate_});
  }
};

REGISTER_CONTROLLER_FACTORY(VidiconDisplayViewFactory);
