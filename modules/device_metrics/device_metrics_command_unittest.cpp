#include "modules/device_metrics/device_metrics_command.h"

#include "address_space/address_space_impl.h"
#include "address_space/address_space_util.h"
#include "address_space/generic_node_factory.h"
#include "address_space/node_factory_util.h"
#include "address_space/standard_address_space.h"
#include "address_space/test/scada_test_address_space.h"
#include "base/check.h"
#include "base/range_util.h"
#include "base/test/awaitable_test.h"
#include "common/node_state.h"
#include "model/devices_node_ids.h"
#include "model/namespaces.h"
#include "modules/device_metrics/node_collector.h"
#include "node_service/test/create_test_node_service.h"
#include "scada/attribute_service_mock.h"
#include "scada/method_service_mock.h"
#include "scada/monitored_item_service_mock.h"

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/map.hpp>
#include <gmock/gmock.h>

#include "base/debug_util.h"

using namespace boost::adaptors;
using namespace testing;

class DeviceMetricsCommandTest : public Test {
 public:
  DeviceMetricsCommandTest();
  ~DeviceMetricsCommandTest();

 protected:
  scada::Node* CreateDevice(scada::NodeId node_id,
                            scada::LocalizedText display_name);
  scada::Node* CreateObject(scada::NodeId node_id,
                            scada::NodeId parent_id,
                            scada::LocalizedText display_name);

  scada::NodeState MakeDataVariableNode() const;

  AddressSpaceImpl address_space_;
  StandardAddressSpace standard_address_space_{address_space_};

  TestExecutor executor_;

  // Lazily materialized from the current address space on first use. All tests
  // create their nodes before the first GetNode() call, so a snapshot taken
  // then reflects the full graph. StaticNodeService answers reads and
  // navigation synchronously (no async fetch), which the former v1 fetcher mock
  // reported immediately as well.
  NodeRef GetNode(const scada::NodeId& node_id) {
    if (!node_service_)
      node_service_ = node_service::test::CreateTestNodeService(address_space_);
    return node_service_->GetNode(node_id);
  }

  std::shared_ptr<NodeService> node_service_;

  const scada::NodeId device_type_definition_id =
      scada::devices::id::Iec60870DeviceType;
  const scada::NamespaceIndex device_namespace_index =
      scada::NamespaceIndexes::IEC60870_DEVICE;
};

MATCHER_P(CellIs, text, "") {
  return arg.GetString16("text") == text;
}

DeviceMetricsCommandTest::DeviceMetricsCommandTest() {
  scada_test::AddScadaDevicesTestTypes(address_space_);
}

DeviceMetricsCommandTest::~DeviceMetricsCommandTest() {
  address_space_.Clear();
}

scada::Node* DeviceMetricsCommandTest::CreateDevice(
    scada::NodeId node_id,
    scada::LocalizedText display_name) {
  GenericNodeFactory node_factory{address_space_};

  auto [status, node] = node_factory.CreateNode(scada::NodeState{
      std::move(node_id), scada::NodeClass::Object, device_type_definition_id,
      scada::devices::id::Devices, scada::id::Organizes,
      scada::NodeAttributes{.display_name = std::move(display_name)}});

  scada::base::Check(status);
  scada::base::Check(node);
  scada::base::Check(node->type_definition());

  CreateDataVariables(node_factory, node->id(), *node->type_definition());

  return node;
}

scada::Node* DeviceMetricsCommandTest::CreateObject(
    scada::NodeId node_id,
    scada::NodeId parent_id,
    scada::LocalizedText display_name) {
  GenericNodeFactory node_factory{address_space_};

  auto [status, node] = node_factory.CreateNode(scada::NodeState{
      std::move(node_id), scada::NodeClass::Object, scada::id::BaseObjectType,
      std::move(parent_id), scada::id::Organizes,
      scada::NodeAttributes{.display_name = std::move(display_name)}});

  scada::base::Check(status);
  scada::base::Check(node);
  return node;
}

TEST_F(DeviceMetricsCommandTest, MakeDeviceMetricsWindowDefinitionSync) {
  const auto device_id1 = scada::NodeId{1, device_namespace_index};
  const auto device_id2 = scada::NodeId{2, device_namespace_index};
  const auto device_id3 = scada::NodeId{3, device_namespace_index};

  const auto* device1 = CreateDevice(device_id1, u"Device 1");
  const auto* device2 = CreateDevice(device_id2, u"Device 2");
  const auto* device3 = CreateDevice(device_id3, u"Device 3");

  const std::u16string title = u"Test title";
  const std::vector devices{
      GetNode(device1->id()),
      GetNode(device2->id()),
      GetNode(device3->id()),
  };

  auto window_definition =
      MakeDeviceMetricsWindowDefinitionSync(title, devices);

  EXPECT_EQ(window_definition.title, title);

  auto rows = window_definition.items |
              filtered([](const WindowItem& window_item) {
                return window_item.name == "SheetCell";
              }) |
              grouped([](const WindowItem& window_item) {
                return window_item.GetInt("row", -1);
              }) |
              map_values | to_vector;

  EXPECT_THAT(
      rows,
      ElementsAre(
          ElementsAre(CellIs(u"Device 1"), CellIs(u"Device 2"),
                      CellIs(u"Device 3")),
          ElementsAre(CellIs(u"Связь"), CellIs(u"={IEC_DEV.1!Online}"),
                      CellIs(u"={IEC_DEV.2!Online}"),
                      CellIs(u"={IEC_DEV.3!Online}")),
          ElementsAre(CellIs(u"Включено"), CellIs(u"={IEC_DEV.1!Enabled}"),
                      CellIs(u"={IEC_DEV.2!Enabled}"),
                      CellIs(u"={IEC_DEV.3!Enabled}")),
          ElementsAre(CellIs(u"Принято сообщений"),
                      CellIs(u"={IEC_DEV.1!MessagesIn}"),
                      CellIs(u"={IEC_DEV.2!MessagesIn}"),
                      CellIs(u"={IEC_DEV.3!MessagesIn}")),
          ElementsAre(CellIs(u"Отправлено сообщений"),
                      CellIs(u"={IEC_DEV.1!MessagesOut}"),
                      CellIs(u"={IEC_DEV.2!MessagesOut}"),
                      CellIs(u"={IEC_DEV.3!MessagesOut}")),
          ElementsAre(CellIs(u"Принято байт"), CellIs(u"={IEC_DEV.1!BytesIn}"),
                      CellIs(u"={IEC_DEV.2!BytesIn}"),
                      CellIs(u"={IEC_DEV.3!BytesIn}")),
          ElementsAre(CellIs(u"Отправлено байт"),
                      CellIs(u"={IEC_DEV.1!BytesOut}"),
                      CellIs(u"={IEC_DEV.2!BytesOut}"),
                      CellIs(u"={IEC_DEV.3!BytesOut}")),
          ElementsAre(CellIs(u"Число синхронизаций времени"),
                      CellIs(u"={IEC_DEV.1!SyncClockCount}"),
                      CellIs(u"={IEC_DEV.2!SyncClockCount}"),
                      CellIs(u"={IEC_DEV.3!SyncClockCount}")),
          ElementsAre(CellIs(u"Число полных опросов"),
                      CellIs(u"={IEC_DEV.1!InterrogateCount}"),
                      CellIs(u"={IEC_DEV.2!InterrogateCount}"),
                      CellIs(u"={IEC_DEV.3!InterrogateCount}"))));
}

TEST_F(DeviceMetricsCommandTest, MakeDeviceMetricsWindowDefinitionAsync) {
  const auto* device1 = CreateDevice({1, device_namespace_index}, u"Device 1");
  const auto* device2 = CreateDevice({2, device_namespace_index}, u"Device 2");

  scada::AddReference(address_space_, scada::id::Organizes, device1->id(),
                      device2->id());

  auto window_definition =
      WaitAwaitable(executor_, MakeDeviceMetricsWindowDefinitionAsync(
                                   executor_, GetNode(device1->id())));

  EXPECT_EQ(window_definition.title, u"Device 1");

  auto header_row = window_definition.items |
                    filtered([](const WindowItem& window_item) {
                      return window_item.name == "SheetCell" &&
                             window_item.GetInt("row", -1) == 1;
                    }) |
                    to_vector;

  EXPECT_THAT(header_row,
              ElementsAre(CellIs(u"Device 1"), CellIs(u"Device 2")));
}

TEST_F(DeviceMetricsCommandTest, CollectChildrenAsyncKeepsOnlyMatchingTypes) {
  const auto* parent = CreateDevice({1, device_namespace_index}, u"Parent");
  const auto* child = CreateDevice({2, device_namespace_index}, u"Child");
  CreateObject({100, device_namespace_index}, parent->id(), u"Folder");

  scada::AddReference(address_space_, scada::id::Organizes, parent->id(),
                      child->id());

  auto children = WaitAwaitable(
      executor_, CollectChildrenAsync(executor_, GetNode(parent->id()),
                                      scada::devices::id::DeviceType));

  ASSERT_THAT(children, SizeIs(1));
  EXPECT_EQ(children.front().node_id(), child->id());
}

TEST_F(DeviceMetricsCommandTest, FetchNodePromiseUsesCoroutineBody) {
  const auto* device = CreateDevice({1, device_namespace_index}, u"Device");

  auto fetched_node =
      WaitAwaitable(executor_, FetchNodeAsync(executor_, GetNode(device->id()),
                                              NodeFetchStatus::NodeOnly));

  EXPECT_EQ(fetched_node.node_id(), device->id());
  EXPECT_TRUE(fetched_node.fetched());
}

TEST_F(DeviceMetricsCommandTest,
       CollectNodesRecursiveAsyncSkipsNonMatchingBranches) {
  const auto* parent = CreateDevice({1, device_namespace_index}, u"Parent");
  const auto* child = CreateDevice({2, device_namespace_index}, u"Child");
  const auto* folder =
      CreateObject({100, device_namespace_index}, parent->id(), u"Folder");
  const auto* skipped_child =
      CreateDevice({3, device_namespace_index}, u"Skipped child");

  scada::AddReference(address_space_, scada::id::Organizes, parent->id(),
                      child->id());
  scada::AddReference(address_space_, scada::id::Organizes, folder->id(),
                      skipped_child->id());

  auto nodes = WaitAwaitable(
      executor_, CollectNodesRecursiveAsync(executor_, GetNode(parent->id()),
                                            scada::devices::id::DeviceType));

  EXPECT_THAT(nodes | transformed(std::mem_fn(&NodeRef::node_id)) | to_vector,
              ElementsAre(parent->id(), child->id()));
}

TEST_F(DeviceMetricsCommandTest, CollectNodesRecursiveAsyncUsesCoroutineBody) {
  const auto* parent = CreateDevice({1, device_namespace_index}, u"Parent");
  const auto* child = CreateDevice({2, device_namespace_index}, u"Child");

  scada::AddReference(address_space_, scada::id::Organizes, parent->id(),
                      child->id());

  auto nodes = WaitAwaitable(
      executor_, CollectNodesRecursiveAsync(executor_, GetNode(parent->id()),
                                            scada::devices::id::DeviceType));

  EXPECT_THAT(nodes | transformed(std::mem_fn(&NodeRef::node_id)) | to_vector,
              ElementsAre(parent->id(), child->id()));
}

TEST_F(DeviceMetricsCommandTest,
       MakeDeviceMetricsWindowDefinitionRejectsNodeWithoutTypeDefinition) {
  EXPECT_THROW(WaitAwaitable(executor_, MakeDeviceMetricsWindowDefinitionAsync(
                                            executor_, NodeRef{})),
               std::runtime_error);
}
