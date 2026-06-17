#include "modus/modus_module.h"

#include "aui/translation.h"
#include "controller/action.h"
#include "controller/command_ui_registry.h"

#include "controller/controller_registry.h"
#include "filesystem/file_registry.h"
#include "modus/modus_component.h"
#include "modus/modus_controller.h"
#include "profile/profile.h"
#include "resources/common_resources.h"

ModusModule::ModusModule(ModusModuleContext&& context)
    : ModusModuleContext{std::move(context)} {
  controller_registry_.AddControllerFactory(
      kModusWindowInfo, [this](const ControllerContext& context) {
        return std::make_unique<ModusController>(context, alias_resolver_);
      });

  file_registry_.RegisterType(kModusWindowInfo.command_id,
                              kModusWindowInfo.name, ".sde;.xsde");

  ui_command_registry_.AddAction(Action{.command_id_ = ID_SETUP,
                                        .category_ = CATEGORY_SETUP,
                                        .title_ = Translate("Options")});

  global_commands_.AddCommand(
      {.title = u"Show Modus topology",
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
      {.title = u"Use Modus runtime renderer",
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

ModusModule::~ModusModule() = default;
