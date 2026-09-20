// Assertions that the fixture's address space carries what the captured
// models read out of it: Explorer sort keys, the event filter's areas, the
// transmission destinations, role membership, and a data group's link state.
//
// These are the ones that fail when screenshot_data.json drifts rather than
// when the client does — a missing reference or attribute renders a plausible
// but empty surface, which no image check can distinguish from a full one.

#include "screenshot_fixture.h"

#include "aui/translation.h"
#include "authenticated_attribute_service.h"
#include "base/utf_convert.h"
#include "common/format.h"
#include "configuration/objects/visible_node_model.h"
#include "events/qt/event_filter_bar.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "model/security_node_ids.h"
#include "modules/limits/limit_model.h"
#include "modules/transmission/transmission_devices.h"
#include "modules/write/write_model.h"
#include "node_service/node_awaitable.h"
#include "node_service/node_fetch_status.h"
#include "node_service/node_ref.h"
#include "node_service/node_util.h"
#include "null_task_manager.h"
#include "profile/profile.h"
#include "profile/window_definition.h"
#include "scada/qualifier.h"
#include "scada/standard_node_ids.h"
#include "scada/status.h"
#include "scada/variant.h"
#include "screenshot_wait.h"
#include "services/device_state_notifier.h"
#include "user_access/role_membership.h"

#include <QApplication>
#include <QLabel>
#include <QLayout>
#include <QStatusBar>
#include <QString>
#include <QToolBar>
#include <QWidget>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace {

using scada::screenshot_generator::FixtureAccountsWith;
using scada::screenshot_generator::FixtureConfig;
using scada::screenshot_generator::ScreenshotGenerator;
using scada::screenshot_generator::ShowMainWindowForMenuCapture;
using scada::screenshot_generator::WaitForAwaitable;
using scada::screenshot_generator::WaitForPendingNodeLoads;

}  // namespace


// The Explorer sorts its rows on NodeClass and TypeDefinition ahead of the
// display name -- folders above leaves, then by type -- so both attributes have
// to be readable for a row before that row is placed. This asserts they are,
// for the children of the Objects root the Explorer is rooted at, which is
// what devices.png renders.
//
// The capture disagrees with them: devices.png comes out as one flat
// alphabetical run, its two leaf rows among the folders rather than below them
// and their Value cells empty (visual_review V43). This test says whether the
// attributes were missing or merely late -- a browse that names a child already
// carries its NodeClass and TypeDefinition, so an unfetched child should still
// classify.
TEST_F(ScreenshotGenerator, ExplorerChildrenCarryTheAttributesTheSortNeeds) {
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));

  NodeService& node_service = app_.node_service();

  // The node the Explorer is rooted at -- the fixture parent of the rows
  // devices.png shows -- fetched with its children the way the tree fetches it.
  // Named from the fixture rather than by a standard id: `ObjectsFolder` holds
  // the SCADA roots, whose own children classify fine, so asking there passes
  // without touching a single row the capture renders.
  NodeRef root = node_service.GetNode(NodeIdFromScadaString("SCADA.24"));
  ASSERT_TRUE(!!root);
  root = WaitForAwaitable(executor_,
                          root.Fetch(NodeFetchStatus::NodeAndChildren));
  ASSERT_TRUE(!!root);

  std::vector<NodeRef> children =
      node_service.GetTargets(root.node_id(), scada::id::Organizes,
                              /*forward=*/true);
  ASSERT_FALSE(children.empty()) << "the Objects root browses to no children";

  // Both sort terms, per child. A child that answers neither is one the
  // comparator cannot place, and every such child ties with every other -- so
  // the comparator falls through to the name and the grouping silently
  // disappears, which is what the capture shows.
  int classified = 0;
  int typed = 0;
  for (const NodeRef& child : children) {
    if (child.node_class().has_value())
      ++classified;
    if (!!child.type_definition())
      ++typed;
  }

  EXPECT_EQ(classified, static_cast<int>(children.size()))
      << "only " << classified << " of " << children.size()
      << " Explorer children report a NodeClass";
  EXPECT_EQ(typed, static_cast<int>(children.size()))
      << "only " << typed << " of " << children.size()
      << " Explorer children resolve a TypeDefinition";
}

// Verifies the event journal's Area filter populates at runtime: the same
// enumeration the filter bar drives (`BrowseEventAreas`), run against the real
// `v1::NodeServiceImpl` over the fixture address space, returns the operator's
// top-level area groupings and drops leaf data items.
TEST_F(ScreenshotGenerator, EventFilterBarEnumeratesAreas) {
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));

  NodeService& node_service = app_.node_service();
  std::vector<EventAreaEntry> areas =
      WaitForAwaitable(executor_, BrowseEventAreas(node_service));

  // The Area dropdown is populated from this list — it must not be empty.
  ASSERT_FALSE(areas.empty());

  // Every enumerated area is a named object grouping, never a leaf data item —
  // that is the level the operator filters the journal by.
  for (const EventAreaEntry& area : areas) {
    EXPECT_FALSE(area.name.empty());
    NodeRef node = node_service.GetNode(area.node_id);
    EXPECT_FALSE(IsInstanceOf(node, scada::data_items::id::DataItemType))
        << "an area must not be a leaf data item";
  }

  // The DataItems root also holds loose top-level data items; the enumeration
  // partitions its Organizes children exactly into areas + excluded leaves.
  std::vector<NodeRef> children = node_service.GetTargets(
      scada::data_items::id::DataItems, scada::id::Organizes, /*forward=*/true);
  size_t leaf_count = 0;
  for (NodeRef& child : children) {
    if (IsInstanceOf(child, scada::data_items::id::DataItemType))
      ++leaf_count;
  }
  EXPECT_EQ(areas.size() + leaf_count, children.size());
}

// Verifies the Transmission view's destination rail populates at runtime: the
// same walk the rail drives (`BrowseTransmissionDevices`), run against the
// real `v1::NodeServiceImpl` over the fixture address space, finds every
// transmission-capable fixture device with its rule count — the plain Modbus
// device (a valid, rule-less destination) and both retransmission devices.
TEST_F(ScreenshotGenerator, DestinationRailEnumeratesTransmissionDevices) {
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));

  NodeService& node_service = app_.node_service();
  std::vector<TransmissionDeviceEntry> devices = WaitForAwaitable(
      executor_, BrowseTransmissionDevices(
                     node_service.GetNode(scada::devices::id::Devices)));

  ASSERT_EQ(devices.size(), 3u);
  EXPECT_EQ(devices[0].node_id, NodeIdFromScadaString("TS.104"));
  EXPECT_EQ(devices[0].rule_count, 0);
  EXPECT_EQ(devices[1].node_id, NodeIdFromScadaString("TS.702"));
  EXPECT_EQ(devices[1].rule_count, 4);
  EXPECT_EQ(devices[2].node_id, NodeIdFromScadaString("TS.703"));
  EXPECT_EQ(devices[2].rule_count, 2);
  for (const TransmissionDeviceEntry& device : devices)
    EXPECT_FALSE(device.name.empty());
}

// Role membership must survive the real node model, not just a static fake.
//
// `ReadRoleMemberships` resolves each rule's Criteria through the TYPE's
// property declarations, so the type chain has to be fetched first. It was
// not, and against the real address space both property lookups returned a
// null NodeRef — every Role came back with no members, which is what
// `roles.png` and the managed `users-admin.png` documented. The module's own
// unit tests could not see it: a StaticNodeService resolves the declaration
// whether or not anything fetched the type.
TEST_F(ScreenshotGenerator, RolesEnumerateTheirMembers) {
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));

  const std::optional<std::vector<RoleMembership>> roles = WaitForAwaitable(
      executor_, ReadRoleMemberships(executor_, app_.node_service(),
                                     authenticated_attribute_service_));
  ASSERT_TRUE(roles.has_value());

  const auto members_of = [&roles](scada::WellKnownRole role) {
    const scada::NodeId id = scada::WellKnownRoleId(role);
    auto i = std::ranges::find(*roles, id, &RoleMembership::node_id);
    return i != roles->end() ? i->members : std::vector<std::u16string>{};
  };

  const std::vector<std::u16string> control =
      FixtureAccountsWith(scada::AccessRight::kControl);
  const std::vector<std::u16string> configure =
      FixtureAccountsWith(scada::AccessRight::kConfigure);
  ASSERT_FALSE(control.empty());
  ASSERT_FALSE(configure.empty());

  EXPECT_THAT(members_of(scada::WellKnownRole::kOperator),
              testing::UnorderedElementsAreArray(control));
  EXPECT_THAT(members_of(scada::WellKnownRole::kConfigureAdmin),
              testing::UnorderedElementsAreArray(configure));
}

// A data group's Value column shows the link state of the device bound to it,
// and the binding is a fixture property no rendered image can be checked
// against: with no HasDevice reference `DataGroupVisibleNode::GetText()`
// returns the empty string, so every group row renders a blank cell and the
// capture still passes check_screenshots.py — which asserts existence,
// dimensions and pairwise distinctness, never text. That is how devices.png
// came to sit beside client.md's sentence «Для групп отображается состояние
// связи с устройством, привязанного к группе» while illustrating nothing.
//
// The state has to be read, not just bound: DeviceStateNotifier addresses the
// device's runtime components as MakeNestedNodeId(device, "Online"), an id no
// fixture node carries, and reaches the fixture's free-standing TS.xxxx
// variable only because GetMutableNestedNode decomposes a nested id back into
// a browse-name child walk (common/address_space/address_space_util.cpp). So
// the assertion is on the rendered text rather than on the reference: a
// resolution path that stopped working would leave the reference in place and
// the cell blank, and the PNG would still be perfectly valid.
TEST_F(ScreenshotGenerator, DataGroupShowsItsDeviceLinkState) {
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));

  // Read the bound groups out of the fixture rather than naming one here, so
  // renaming or re-binding the group cannot leave this test asserting a
  // binding nothing carries.
  std::vector<scada::NodeId> bound_groups;
  for (const auto& node : FixtureConfig().json.at("nodes").as_array()) {
    const auto& object = node.as_object();
    const auto* references = object.if_contains("references");
    if (!references)
      continue;
    for (const auto& reference : references->as_array()) {
      if (NodeIdFromScadaString(
              std::string_view(reference.as_object().at("type").as_string())) ==
          scada::data_items::id::HasDevice) {
        bound_groups.push_back(NodeIdFromScadaString(
            std::string_view(object.at("id").as_string())));
      }
    }
  }
  ASSERT_FALSE(bound_groups.empty())
      << "no fixture group carries a HasDevice reference, so the object "
         "tree's Value column is blank on every group row";

  ASSERT_TRUE(scada::screenshot_generator::FetchNodesResident(
      executor_, app_.node_service(), bound_groups));

  for (const scada::NodeId& group_id : bound_groups) {
    SCOPED_TRACE(NodeIdToScadaString(group_id));

    NodeRef group = app_.node_service().GetNode(group_id);
    ASSERT_TRUE(group);
    ASSERT_TRUE(IsInstanceOf(group, scada::data_items::id::DataGroupType))
        << "only a DataGroupType instance gets a DataGroupVisibleNode; on any "
           "other node the HasDevice reference is dead weight";

    DataGroupVisibleNode visible_node{app_.timed_data_service(), group};

    // The device's Online component is subscribed through TimedDataService, so
    // its first value lands on a later turn of the loop.
    scada::screenshot_generator::PumpEventLoopFor(
        std::chrono::milliseconds{500});

    EXPECT_EQ(visible_node.GetText(), ToLocalizedString(DeviceState::Online))
        << "the group reads «"
        << QString::fromStdU16String(visible_node.GetText()).toStdString()
        << "». An empty cell means the state never resolved — either the "
           "HasDevice target is unreachable, or its Online component is not "
           "at MakeNestedNodeId(device, \"Online\") where the notifier looks. "
           "The fixture binds the online device on purpose: hardware-tree.png "
           "is what shows the offline and disabled states.";
  }
}
