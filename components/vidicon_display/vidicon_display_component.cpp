#include "components/vidicon_display/vidicon_display_component.h"

#include "components/vidicon_display/vidicon_display_view.h"
#include "controller_context.h"
#include "controller_registry.h"

const WindowInfo kVidiconDisplayWindowInfo = {
    ID_VIDICON_DISPLAY_VIEW, "VidiconDisplay", u"Схема", 0, 0, 0, 0};

class VidiconDisplayViewFactory : public ControllerRegistrarBase {
 public:
  VidiconDisplayViewFactory()
      : ControllerRegistrarBase{kVidiconDisplayWindowInfo} {}

  virtual std::unique_ptr<Controller> CreateController(
      const ControllerContext& context) override {
    return std::make_unique<VidiconDisplayView>(context.vidicon_client_);
  }
};

REGISTER_CONTROLLER_FACTORY(VidiconDisplayViewFactory);
