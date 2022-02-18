#include "services/property_defs.h"

#include "address_space/address_space_impl3.h"
#include "address_space/address_space_util.h"
#include "address_space/generic_node_factory.h"
#include "address_space/node_factory_util.h"
#include "common/formula_util.h"
#include "common/node_state.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/namespaces.h"
#include "model/node_id_util.h"
#include "node_service/v1/test/test_node_service.h"
#include "services/dialog_service_mock.h"
#include "services/task_manager_mock.h"

#include "base/debug_util-inl.h"

using namespace testing;

TEST(ChannelPropertyDefinition, GetText_Device) {
  const scada::NodeId node_id{1, 1};
  const scada::NodeId device_id{1, NamespaceIndexes::IEC61850_DEVICE};
  const scada::NodeId prop_decl_id = data_items::id::DataItemType_Input1;

  AddressSpaceImpl3 address_space;
  GenericNodeFactory node_factory{address_space};
  node_factory.CreateNode(
      scada::NodeState{}
          .set_node_id(device_id)
          .set_node_class(scada::NodeClass::Object)
          .set_type_definition_id(devices::id::Iec61850DeviceType)
          .set_parent(scada::id::Organizes, devices::id::Devices)
          .set_attributes(
              scada::NodeAttributes{}.set_display_name(u"DeviceDisplayName")));
  auto [status, node_ptr] = node_factory.CreateNode(
      scada::NodeState{}
          .set_node_id(node_id)
          .set_node_class(scada::NodeClass::Variable)
          .set_type_definition_id(data_items::id::DiscreteItemType)
          .set_parent(scada::id::Organizes, data_items::id::DataItems));
  ASSERT_TRUE(status);
  ASSERT_TRUE(node_ptr);
  scada::SetPropertyValue(
      *node_ptr, prop_decl_id,
      MakeNodeIdFormula(MakeNestedNodeId(device_id, "device.channel.path")));

  auto node_service = v1::CreateTestNodeService(address_space);
  const auto& node = node_service->GetNode(node_id);

  ChannelPropertyDefinition channel_property_definition{u"Title", true};

  StrictMock<MockTaskManager> task_manager;
  StrictMock<MockDialogService> dialog_service;
  PropertyContext property_context{*node_service, task_manager, dialog_service};

  const std::u16string expected_text = u"DeviceDisplayName";
  EXPECT_EQ(expected_text, channel_property_definition.GetText(
                               property_context, node, prop_decl_id));
}
