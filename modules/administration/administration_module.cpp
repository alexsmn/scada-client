#include "administration/administration_module.h"

#include "administration/administration_view.h"
#include "user_access/qt/password_policy_view.h"
#include "user_access/qt/roles_view.h"
#include "controller/controller_registry.h"
#include "controller/window_info.h"
#include "resources/common_resources.h"

namespace {

// WIN_SING: a left-dock pane, which is what a rail mode switches between.
// WIN_REQUIRES_ADMIN both gates the pane and, through
// MainWindow::IsPaneModeAvailable, hides the whole Administration rail mode
// from a session without the Configure right.
constexpr WindowInfo kAdministrationWindowInfo = {
    .command_id = ID_ADMINISTRATION_VIEW,
    .name = "Administration",
    .title = u"Administration",
    .flags = WIN_SING | WIN_REQUIRES_ADMIN,
    .size = {200, 400}};

// The Roles workspace view. Registered here rather than beside its own class:
// a REGISTER_CONTROLLER in a static-library TU nothing references is dropped
// by the linker, and this module is the one that routes to it.
constexpr WindowInfo kRolesWindowInfo = {.command_id = ID_ROLES_VIEW,
                                         .name = "Roles",
                                         .title = u"Roles",
                                         .flags = WIN_REQUIRES_ADMIN};

constexpr WindowInfo kPasswordPolicyWindowInfo = {
    .command_id = ID_PASSWORD_POLICY_VIEW,
    .name = "PasswordPolicy",
    .title = u"Password policy",
    .flags = WIN_REQUIRES_ADMIN};

}  // namespace

AdministrationModule::AdministrationModule(
    AdministrationModuleContext&& context)
    : context_{std::move(context)} {
  context_.controller_registry_.AddControllerFactory(
      kAdministrationWindowInfo, [](const ControllerContext& context) {
        return std::make_unique<AdministrationView>(context);
      });

  context_.controller_registry_.AddControllerFactory(
      kRolesWindowInfo, [](const ControllerContext& context) {
        return std::make_unique<RolesView>(context);
      });

  context_.controller_registry_.AddControllerFactory(
      kPasswordPolicyWindowInfo, [](const ControllerContext& context) {
        return std::make_unique<PasswordPolicyView>(context);
      });
}

AdministrationModule::~AdministrationModule() = default;
