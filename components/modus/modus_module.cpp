#include "modus/modus_module.h"

#include "controller/controller_registry.h"
#include "filesystem/file_registry.h"
#include "modus/libmodus/modus_module2.h"
#include "modus/modus_component.h"
#include "modus/modus_controller.h"
#include "profile/profile.h"

ModusModule::ModusModule(ModusModuleContext&& context)
    : ModusModuleContext{std::move(context)} {
  modus_module_ = std::make_unique<ModusModule2>(blinker_manager_);
  ModusModule2::SetInstance(modus_module_.get());

  controller_registry_.AddControllerFactory(
      kModusWindowInfo, [this](const ControllerContext& context) {
        return std::make_unique<ModusController>(context, alias_resolver_);
      });

  file_registry_.RegisterType(kModusWindowInfo.command_id,
                              kModusWindowInfo.name, ".sde;.xsde");

  global_commands_.AddCommand(
      {.title = u"Отображать топологию на схемах Modus",
       .menu_group = MenuGroup::DISPLAY_SETTINGS,
       .execute_handler =
           [&profile = profile_](const GlobalCommandContext& context) {
             profile.modus.topology = !profile.modus.topology;
             profile.NotifyChange();
           },
       .checked_handler =
           [&profile = profile_](const GlobalCommandContext& context) {
             return profile.modus.topology;
           }});

  global_commands_.AddCommand(
      {.title = u"Встроенный визуализатор схем Modus",
       .menu_group = MenuGroup::DISPLAY_SETTINGS,
       .execute_handler =
           [&profile = profile_](const GlobalCommandContext& context) {
             profile.modus.modus2 = !profile.modus.modus2;
             profile.NotifyChange();
           },
       .checked_handler =
           [&profile = profile_](const GlobalCommandContext& context) {
             return profile.modus.modus2;
           }});
}

ModusModule::~ModusModule() {
  ModusModule2::SetInstance(nullptr);
}
