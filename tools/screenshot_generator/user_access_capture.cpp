#include "user_access_capture.h"

#include "publish_guard.h"
#include "screenshot_config.h"
#include "screenshot_wait.h"
#include "widget_capture.h"

#include "account_roles/account_roles_dialog.h"
#include "account_roles/qt/account_roles_dialog_qt.h"
#include "model/node_id_util.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "scada/node_id.h"
#include "user_access/qt/roles_grid_panel.h"
#include "user_access/qt/user_access_panel.h"
#include "user_access/qt/users_grid_panel.h"
#include "user_access/role_membership.h"
#include "user_access/users_grid.h"

#include <gtest/gtest.h>

#include <array>
#include <memory>

void SaveUserAccessScreenshot(const ScreenshotSpec& spec,
                              NodeService& node_service,
                              scada::AttributeService& attribute_service,
                              AnyExecutor executor) {
  // USER.5 "Администратор" — the mockup's user. The panel reads its Roles from
  // the RoleSet; the node supplies only the account NAME they are matched by.
  const scada::NodeId user_id = NodeIdFromScadaString("USER.5");

  // Make the user resident so the panel can read its display name.
  const std::array<scada::NodeId, 1> ids{user_id};
  scada::screenshot_generator::FetchNodesResident(executor, node_service, ids);

  NodeRef user = node_service.GetNode(user_id);

  UserAccessPanel panel;
  panel.ShowUser(user, node_service, attribute_service, executor);

  SaveScreenshot(&panel, spec);
}

void SaveUsersGridScreenshot(const ScreenshotSpec& spec,
                             NodeService& node_service,
                             scada::AttributeService& attribute_service,
                             AnyExecutor executor) {
  CapturePublishGuard publish_guard{spec.filename};

  std::unique_ptr<UsersGridPanel> panel{MakeUsersGridPanel()};

  const std::optional<std::vector<UserGridRow>> rows =
      scada::screenshot_generator::WaitForAwaitable(
          executor, BuildUsersGrid(executor, node_service, attribute_service));
  if (rows) {
    panel->ShowRows(*rows);
  } else {
    ADD_FAILURE() << spec.filename
                  << ": the users grid could not be read at all";
  }

  if (!publish_guard.ShouldPublish())
    return;

  SaveScreenshot(panel.get(), spec);
}

void SaveRolesScreenshot(const ScreenshotSpec& spec,
                         NodeService& node_service,
                         scada::AttributeService& attribute_service,
                         AnyExecutor executor) {
  CapturePublishGuard publish_guard{spec.filename};

  std::unique_ptr<RolesGridPanel> panel{MakeRolesGridPanel()};

  // Await the read instead of spawning it: the view path spawns and the
  // capture then raced it, grabbing a grid that had not been filled yet.
  panel->ShowRoles(scada::screenshot_generator::WaitForAwaitable(
      executor,
      ReadRoleMemberships(executor, node_service, attribute_service)));

  SaveScreenshot(panel.get(), spec);
}

void SaveAccountRolesScreenshot(const ScreenshotSpec& spec,
                                NodeService& node_service,
                                scada::AttributeService& attribute_service,
                                AnyExecutor executor) {
  CapturePublishGuard publish_guard{spec.filename};

  // The fixture administrator, as in the users-rbac capture: it holds Operator
  // and ConfigureAdmin, so the dialog shows both a checked and an unchecked
  // state.
  const std::u16string account = u"Администратор";
  const auto roles = scada::screenshot_generator::WaitForAwaitable(
      executor, ReadRoleMemberships(executor, node_service, attribute_service));
  if (!roles) {
    ADD_FAILURE() << spec.filename << ": the RoleSet could not be read";
    return;
  }

  AccountRolesDialog dialog{account, MakeAccountRoleChoices(*roles, account)};

  if (!publish_guard.ShouldPublish())
    return;

  SaveScreenshot(&dialog, spec);
}
