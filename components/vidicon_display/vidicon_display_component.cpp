#include "controller_factory.h"

#if defined(UI_QT)
#include "components/vidicon_display/qt/vidicon_display_view.h"
#elif defined(UI_VIEWS)
#include "components/vidicon_display/views/vidicon_display_view.h"
#endif

const WindowInfo kWindowInfo = {
    ID_VIDICON_DISPLAY_VIEW, "VidiconDisplay", L"Схема", 0, 0, 0, 0};

class VidiconDisplayViewFactory : public ControllerRegistrarBase {
 public:
  VidiconDisplayViewFactory() : ControllerRegistrarBase{kWindowInfo} {}

  virtual std::unique_ptr<Controller> CreateController(
      const ControllerContext& context) override {
    return std::make_unique<VidiconDisplayView>();
  }
};

REGISTER_CONTROLLER_FACTORY(VidiconDisplayViewFactory);
// REGISTER_CONTROLLER(VidiconDisplayView, kWindowInfo);
