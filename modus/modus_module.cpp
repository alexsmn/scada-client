#include "modus/modus_module.h"

#include "controller/controller_registry.h"
#include "filesystem/file_registry.h"
#include "modus/libmodus/modus_module2.h"
#include "modus/modus_component.h"
#include "modus/modus_controller.h"

ModusModule::ModusModule(ModusModuleContext&& context)
    : ModusModuleContext{std::move(context)} {
  modus_module_ = std::make_unique<ModusModule2>(blinker_manager_);
  ModusModule2::SetInstance(modus_module_.get());

  controller_registry_.AddControllerFactory(
      kModusWindowInfo, [](const ControllerContext& context) {
        return std::make_unique<ModusController>(context);
      });

  file_registry_.RegisterType(kModusWindowInfo.command_id,
                              kModusWindowInfo.name, ".sde;.xsde");
}

ModusModule::~ModusModule() {
  ModusModule2::SetInstance(nullptr);
  modus_module_.reset();
}
