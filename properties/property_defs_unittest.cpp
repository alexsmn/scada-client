#include "properties/property_defs.h"

#include "address_space/address_space_impl3.h"
#include "address_space/address_space_util.h"
#include "address_space/generic_node_factory.h"
#include "address_space/node_factory_util.h"
#include "aui/dialog_service_mock.h"
#include "base/u16format.h"
#include "common/formula_util.h"
#include "common/node_state.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/namespaces.h"
#include "model/node_id_util.h"
#include "node_service/node_util.h"
#include "node_service/v1/test/test_node_service.h"
#include "properties/channel_property_definition.h"
#include "properties/property_context.h"
#include "properties/property_defs.h"
#include "properties/property_service.h"
#include "services/task_manager_mock.h"

#include "base/debug_util-inl.h"

using namespace testing;

class PropertyDefsTest : public Test {
 protected:
  PropertyDefsTest();

  NodeRef CreateDataItem(std::string_view channel_path);

  AddressSpaceImpl3 address_space;
  GenericNodeFactory node_factory{address_space};

  std::shared_ptr<NodeService> node_service =
      v1::CreateTestNodeService(address_space);

  StrictMock<MockTaskManager> task_manager;
  StrictMock<MockDialogService> dialog_service;
  PropertyContext property_context{*node_service, task_manager, dialog_service};

  ChannelPropertyDefinition channel_property_definition{u"Title", true};

  inline static const scada::NodeId data_item_id{1, 1};
  inline static const scada::NodeId data_group_id{2, 1};
  inline static const scada::NodeId link_id{1, NamespaceIndexes::IEC60870_LINK};
  inline static const scada::NodeId device_id{
      1, NamespaceIndexes::IEC60870_DEVICE};
  inline static const char16_t kLinkDisplayName[] = u"LinkDisplayName";
  inline static const char16_t kDeviceDisplayName[] = u"DeviceDisplayName";
};

PropertyDefsTest::PropertyDefsTest() {
  // Create Link.
  node_factory.CreateNode(
      scada::NodeState{}
          .set_node_id(link_id)
          .set_node_class(scada::NodeClass::Object)
          .set_type_definition_id(devices::id::Iec60870LinkType)
          .set_parent(scada::id::Organizes, devices::id::Devices)
          .set_attributes(
              scada::NodeAttributes{}.set_display_name(kLinkDisplayName))
          .set_properties(scada::NodeProperties{
              {devices::id::Iec60870LinkType_Mode, scada::Variant{0}}}));

  // Create Device.
  node_factory.CreateNode(
      scada::NodeState{}
          .set_node_id(device_id)
          .set_node_class(scada::NodeClass::Object)
          .set_type_definition_id(devices::id::Iec60870DeviceType)
          .set_parent(scada::id::Organizes, link_id)
          .set_attributes(
              scada::NodeAttributes{}.set_display_name(kDeviceDisplayName)));

  // Create Data Group.
  {
    auto [status, node_ptr] = node_factory.CreateNode(
        scada::NodeState{}
            .set_node_id(data_group_id)
            .set_node_class(scada::NodeClass::Object)
            .set_type_definition_id(data_items::id::DataGroupType)
            .set_parent(scada::id::Organizes, data_items::id::DataItems));
    assert(status);
    assert(node_ptr);

    scada::AddReference(address_space, data_items::id::HasDevice, data_group_id,
                        device_id);
  }
}

NodeRef PropertyDefsTest::CreateDataItem(std::string_view channel_path) {
  auto [status, node_ptr] = node_factory.CreateNode(
      scada::NodeState{}
          .set_node_id(data_item_id)
          .set_node_class(scada::NodeClass::Variable)
          .set_type_definition_id(data_items::id::DiscreteItemType)
          .set_parent(scada::id::Organizes, data_group_id));
  assert(status);
  assert(node_ptr);

  scada::SetPropertyValue(*node_ptr, data_items::id::DataItemType_Input1,
                          scada::String{channel_path});

  return node_service->GetNode(data_item_id);
}

TEST_F(PropertyDefsTest, GetText_Device) {
  auto data_item_node = CreateDataItem(
      MakeNodeIdFormula(MakeNestedNodeId(device_id, "device.channel.path")));

  EXPECT_EQ(
      u16format(L"{} : {}", kLinkDisplayName, kDeviceDisplayName),
      channel_property_definition.GetText(property_context, data_item_node,
                                          data_items::id::DataItemType_Input1));
}

TEST_F(PropertyDefsTest, GetText_GroupDevice) {
  auto data_item_node = CreateDataItem("GROUP_DEVICE!device.channel.path");

  EXPECT_EQ(
      ChannelPropertyDefinition::kParentGroupDevice,
      channel_property_definition.GetText(property_context, data_item_node,
                                          data_items::id::DataItemType_Input1));
}

TEST_F(PropertyDefsTest, Enum) {
  auto mode_prop_def =
      node_service->GetNode(devices::id::Iec60870LinkType_Mode);
  auto mode_data_type = mode_prop_def.data_type();
  ASSERT_TRUE(IsSubtypeOf(mode_data_type, scada::id::Enumeration));

  auto link = node_service->GetNode(link_id);
  auto* prop_def = PropertyService{}.GetPropertyDef(mode_prop_def);
  ASSERT_TRUE(prop_def);
  EXPECT_EQ(u"Polling",
            prop_def->GetText(property_context, link, mode_prop_def.node_id()));
}
