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
                              NodeService& node_service,
                              scada::AttributeService& attribute_service,
                              AnyExecutor executor) {
  // USER.5 "Администратор" — the mockup's user. The panel reads its Roles from
  // the RoleSet; the node supplies only the account NAME they are matched by.
  const scada::NodeId user_id = NodeIdFromScadaString("USER.5");

  // Make the user resident so the panel can read its display name.
  const std::array<scada::NodeId, 1> ids{user_id};
  scada::screenshot_generator::FetchNodesResident(node_service, ids);

  NodeRef user = node_service.GetNode(user_id);

  UserAccessPanel panel;
  panel.ShowUser(user, node_service, attribute_service, executor);

  SaveScreenshot(&panel, spec);
}
