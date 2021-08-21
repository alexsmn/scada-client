#include "components/device_metrics/device_metrics_command.h"

#include "address_space/address_space_impl.h"
#include "address_space/generic_node_factory.h"
#include "address_space/node_factory_util.h"
#include "address_space/scada_address_space.h"
#include "address_space/standard_address_space.h"
#include "base/range_util.h"
#include "base/test/test_executor.h"
#include "common/node_state.h"
#include "core/attribute_service_mock.h"
#include "core/method_service_mock.h"
#include "core/monitored_item_service_mock.h"
#include "model/devices_node_ids.h"
#include "model/namespaces.h"
#include "node_service/v1/address_space_fetcher_mock.h"
#include "node_service/v1/node_service_impl.h"

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/map.hpp>
#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

using namespace boost::adaptors;
using namespace testing;

class DeviceMetricsCommandTest : public Test {
 public:
  DeviceMetricsCommandTest();
  ~DeviceMetricsCommandTest();

 protected:
  scada::Node* CreateDevice(scada::NodeId node_id,
                            scada::LocalizedText display_name);

  scada::NodeState MakeDataVariableNode() const;

  const std::shared_ptr<Executor> executor_ = std::make_shared<TestExecutor>();

  AddressSpaceImpl address_space_;
  StandardAddressSpace standard_address_space_{address_space_};

  const std::shared_ptr<v1::MockAddressSpaceFetcher> address_space_fetcher_ =
      std::make_shared<NiceMock<v1::MockAddressSpaceFetcher>>();

  StrictMock<scada::MockAttributeService> attribute_service_;
  StrictMock<scada::MockMonitoredItemService> monitored_item_service_;
  StrictMock<scada::MockMethodService> method_service_;

  v1::NodeServiceImpl node_service_{v1::NodeServiceImplContext{
      executor_, MakeAddressSpaceFetcherFactory(), address_space_,
      attribute_service_, monitored_item_service_, method_service_}};

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
  CreateScadaAddressSpace(address_space_, node_factory);
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

TEST_F(DeviceMetricsCommandTest, MakeDeviceMetricsWindowDefinitionSync) {
  const auto device_id1 = scada::NodeId{1, device_namespace_index};
  const auto device_id2 = scada::NodeId{2, device_namespace_index};
  const auto device_id3 = scada::NodeId{3, device_namespace_index};

  auto* device1 = CreateDevice(device_id1, L"Device 1");
  auto* device2 = CreateDevice(device_id2, L"Device 2");
  auto* device3 = CreateDevice(device_id3, L"Device 3");

  const std::wstring title = L"Test title";
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
          ElementsAre(CellIs(L"Device 1"), CellIs(L"Device 2"),
                      CellIs(L"Device 3")),
          ElementsAre(CellIs(L"Связь"), CellIs(L"={IEC_DEV.1!Online}"),
                      CellIs(L"={IEC_DEV.2!Online}"),
                      CellIs(L"={IEC_DEV.3!Online}")),
          ElementsAre(CellIs(L"Включено"), CellIs(L"={IEC_DEV.1!Enabled}"),
                      CellIs(L"={IEC_DEV.2!Enabled}"),
                      CellIs(L"={IEC_DEV.3!Enabled}")),
          ElementsAre(CellIs(L"Принято сообщений"),
                      CellIs(L"={IEC_DEV.1!MessagesIn}"),
                      CellIs(L"={IEC_DEV.2!MessagesIn}"),
                      CellIs(L"={IEC_DEV.3!MessagesIn}")),
          ElementsAre(CellIs(L"Отправлено сообщений"),
                      CellIs(L"={IEC_DEV.1!MessagesOut}"),
                      CellIs(L"={IEC_DEV.2!MessagesOut}"),
                      CellIs(L"={IEC_DEV.3!MessagesOut}")),
          ElementsAre(CellIs(L"Принято байт"), CellIs(L"={IEC_DEV.1!BytesIn}"),
                      CellIs(L"={IEC_DEV.2!BytesIn}"),
                      CellIs(L"={IEC_DEV.3!BytesIn}")),
          ElementsAre(CellIs(L"Отправлено байт"),
                      CellIs(L"={IEC_DEV.1!BytesOut}"),
                      CellIs(L"={IEC_DEV.2!BytesOut}"),
                      CellIs(L"={IEC_DEV.3!BytesOut}"))));
}
