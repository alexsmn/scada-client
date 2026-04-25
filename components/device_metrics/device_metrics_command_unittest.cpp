#include "components/device_metrics/device_metrics_command.h"

#include "address_space/address_space_impl.h"
#include "address_space/address_space_util.h"
#include "address_space/generic_node_factory.h"
#include "address_space/node_factory_util.h"
#include "address_space/scada_address_space.h"
#include "address_space/standard_address_space.h"
#include "base/test/awaitable_test.h"
#include "base/range_util.h"
#include "common/node_state.h"
#include "components/device_metrics/node_collector.h"
#include "scada/attribute_service_mock.h"
#include "scada/method_service_mock.h"
#include "scada/monitored_item_service_mock.h"
#include "model/devices_node_ids.h"
#include "model/namespaces.h"
#include "node_service/v1/address_space_fetcher_mock.h"
#include "node_service/v1/node_service_impl.h"

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

  const std::shared_ptr<v1::MockAddressSpaceFetcher> address_space_fetcher_ =
      std::make_shared<NiceMock<v1::MockAddressSpaceFetcher>>();

  StrictMock<scada::MockAttributeService> attribute_service_;
  StrictMock<scada::MockMonitoredItemService> monitored_item_service_;
  StrictMock<scada::MockMethodService> method_service_;

  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();

  v1::NodeServiceImpl node_service_{v1::NodeServiceImplContext{
      MakeAddressSpaceFetcherFactory(), address_space_, attribute_service_,
      monitored_item_service_, method_service_}};

  const scada::NodeId device_type_definition_id =
      devices::id::Iec60870DeviceType;
  const scada::NamespaceIndex device_namespace_index =
      NamespaceIndexes::IEC60870_DEVICE;

 private:
  v1::AddressSpaceFetcherFactory MakeAddressSpaceFetcherFactory() {
    return [address_space_fetcher = address_space_fetcher_](
               v1::AddressSpaceFetcherFactoryContext&& context) {
      return address_space_fetcher;
    };
  }
};

MATCHER_P(CellIs, text, "") {
  return arg.GetString16("text") == text;
}

DeviceMetricsCommandTest::DeviceMetricsCommandTest() {
  ON_CALL(*address_space_fetcher_, GetNodeFetchStatus(_))
      .WillByDefault(Return(std::make_pair(
          scada::StatusCode::Good, NodeFetchStatus::NodeAndChildren())));

  GenericNodeFactory node_factory{address_space_};
  ScadaAddressSpaceBuilder{address_space_, node_factory}.BuildAll();
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
      devices::id::Devices, scada::id::Organizes,
      scada::NodeAttributes{}.set_display_name(std::move(display_name))});

  assert(status);
  assert(node);
  assert(node->type_definition());

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
      scada::NodeAttributes{}.set_display_name(std::move(display_name))});

  assert(status);
  assert(node);
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
      node_service_.GetNode(device1->id()),
      node_service_.GetNode(device2->id()),
      node_service_.GetNode(device3->id()),
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

  auto window_definition = WaitAwaitable(
      executor_, MakeDeviceMetricsWindowDefinitionAsync(
                     MakeTestAnyExecutor(executor_),
                     node_service_.GetNode(device1->id())));

  EXPECT_EQ(window_definition.title, u"Device 1");

  auto header_row =
      window_definition.items |
      filtered([](const WindowItem& window_item) {
        return window_item.name == "SheetCell" &&
               window_item.GetInt("row", -1) == 1;
      }) |
      to_vector;

  EXPECT_THAT(header_row, ElementsAre(CellIs(u"Device 1"),
                                      CellIs(u"Device 2")));
}

TEST_F(DeviceMetricsCommandTest, CollectChildrenAsyncKeepsOnlyMatchingTypes) {
  const auto* parent = CreateDevice({1, device_namespace_index}, u"Parent");
  const auto* child = CreateDevice({2, device_namespace_index}, u"Child");
  CreateObject({100, device_namespace_index}, parent->id(), u"Folder");

  scada::AddReference(address_space_, scada::id::Organizes, parent->id(),
                      child->id());

  auto children = WaitAwaitable(
      executor_, CollectChildrenAsync(MakeTestAnyExecutor(executor_),
                                      node_service_.GetNode(parent->id()),
                                      devices::id::DeviceType));

  ASSERT_THAT(children, SizeIs(1));
  EXPECT_EQ(children.front().node_id(), child->id());
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
      executor_, CollectNodesRecursiveAsync(MakeTestAnyExecutor(executor_),
                                            node_service_.GetNode(parent->id()),
                                            devices::id::DeviceType));

  EXPECT_THAT(nodes | transformed(std::mem_fn(&NodeRef::node_id)) | to_vector,
              ElementsAre(parent->id(), child->id()));
}

TEST_F(DeviceMetricsCommandTest, CollectNodesRecursivePromiseUsesCoroutineBody) {
  const auto* parent = CreateDevice({1, device_namespace_index}, u"Parent");
  const auto* child = CreateDevice({2, device_namespace_index}, u"Child");

  scada::AddReference(address_space_, scada::id::Organizes, parent->id(),
                      child->id());

  auto nodes = CollectNodesRecursive(node_service_.GetNode(parent->id()),
                                     devices::id::DeviceType)
                   .get();

  EXPECT_THAT(nodes | transformed(std::mem_fn(&NodeRef::node_id)) | to_vector,
              ElementsAre(parent->id(), child->id()));
}

TEST_F(DeviceMetricsCommandTest,
       MakeDeviceMetricsWindowDefinitionRejectsNodeWithoutTypeDefinition) {
  const auto promise = MakeDeviceMetricsWindowDefinition(
      MakeTestAnyExecutor(executor_), NodeRef{});

  EXPECT_THROW(WaitPromise(executor_, promise), std::runtime_error);
}
