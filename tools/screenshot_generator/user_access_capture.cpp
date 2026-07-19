#include "user_access_capture.h"

#include "screenshot_config.h"
#include "screenshot_wait.h"
#include "widget_capture.h"

#include "model/node_id_util.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "scada/node_id.h"
#include "user_access/qt/user_access_panel.h"

#include <array>

void SaveUserAccessScreenshot(const ScreenshotSpec& spec,
                              NodeService& node_service) {
  // USER.5 "Администратор" carries AccessRights = 3 (Configure + Control), i.e.
  // the Administrator role with every permission granted — the mockup's user.
  const scada::NodeId user_id = NodeIdFromScadaString("USER.5");

  // Make the user (its type + AccessRights property) resident so ShowUser can
  // resolve the AccessRights aggregate and read its value.
  const std::array<scada::NodeId, 1> ids{user_id};
  scada::screenshot_generator::FetchNodesResident(node_service, ids);

  NodeRef user = node_service.GetNode(user_id);

  UserAccessPanel panel;
  panel.ShowUser(user);

  SaveScreenshot(&panel, spec);
}
