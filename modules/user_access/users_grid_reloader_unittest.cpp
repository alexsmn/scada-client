#include "user_access/users_grid_reloader.h"

#include "model/namespaces.h"
#include "node_service/test/fake_node_service.h"
#include "scada/authorization.h"
#include "scada/event.h"

#include <gtest/gtest.h>

namespace {

// An account, a Role or a role rule changing reloads the Users view; anything
// else leaves it alone (backlog 149).
TEST(UsersGridReloaderTest, ReloadsOnAccountAndRoleChangesOnly) {
  FakeNodeService node_service;
  int reloads = 0;
  UsersGridReloader reloader{node_service, [&] { ++reloads; }};

  // A renamed or disabled account.
  node_service.EmitNodeSemanticChanged(
      scada::NodeId{13, scada::NamespaceIndexes::USER});
  EXPECT_EQ(reloads, 1);
  // A created or deleted account, and a granted or revoked Role.
  node_service.EmitModelChanged(scada::ModelChangeEvent{
      .node_id = scada::NodeId{14, scada::NamespaceIndexes::USER},
      .verb = scada::ModelChangeEvent::NodeAdded});
  node_service.EmitModelChanged(scada::ModelChangeEvent{
      .node_id = scada::NodeId{3, scada::NamespaceIndexes::ROLE_IDENTITY},
      .verb = scada::ModelChangeEvent::NodeDeleted});
  EXPECT_EQ(reloads, 3);

  // A data item or a device is none of its business.
  node_service.EmitNodeSemanticChanged(
      scada::NodeId{1, scada::NamespaceIndexes::TS});
  node_service.EmitModelChanged(scada::ModelChangeEvent{
      .node_id = scada::NodeId{3, scada::NamespaceIndexes::IEC60870_LINK},
      .verb = scada::ModelChangeEvent::NodeAdded});
  EXPECT_EQ(reloads, 3);
}

// Closing the view stops the reloads.
TEST(UsersGridReloaderTest, StopsWhenDestroyed) {
  FakeNodeService node_service;
  int reloads = 0;
  {
    UsersGridReloader reloader{node_service, [&] { ++reloads; }};
  }
  node_service.EmitNodeSemanticChanged(
      scada::NodeId{13, scada::NamespaceIndexes::USER});
  EXPECT_EQ(reloads, 0);
}

}  // namespace
