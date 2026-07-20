#include "properties/property_defs.h"

#include "address_space/address_space_util.h"
#include "address_space/generic_node_factory.h"
#include "address_space/node_factory_util.h"
#include "address_space/test/scada_test_address_space.h"
#include "address_space/type_definition.h"
#include "aui/dialog_service_mock.h"
#include "base/check.h"
#include "base/test/awaitable_test.h"
#include "base/u16format.h"
#include "common/formula_util.h"
#include "common/node_state.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/namespaces.h"
#include "model/node_id_util.h"
#include "node_service/node_util.h"
#include "node_service/test/create_test_node_service.h"
#include "properties/channel_property_definition.h"
#include "properties/property_context.h"
#include "properties/property_defs.h"
#include "properties/property_service.h"
#include "properties/property_util.h"
#include "services/task_manager_mock.h"

#include "base/debug_util.h"

#include <algorithm>

using namespace testing;

namespace {

// Adds the IEC 60870 link/device type system that PropertyDefsTest exercises.
// These types live in the server-owned `devices_iec60870` nodeset; they are
// reproduced here in code so the client test needs no nodeset XML. Display names
// and the Mode enum strings mirror that nodeset.
void AddIec60870TestTypes(AddressSpaceImpl& address_space) {
  GenericNodeFactory factory{address_space};
  namespace dev = scada::devices::id;

  // Base LinkType : DeviceType (DeviceType comes from ScadaTestAddressSpace).
  factory.CreateNode(
      scada::NodeState{.node_id = dev::LinkType,
                       .node_class = scada::NodeClass::ObjectType,
                       .parent_id = dev::DeviceType,
                       .reference_type_id = {scada::id::HasSubtype,
                                             scada::NamespaceIndexes::NS0},
                       .attributes = scada::NodeAttributes{}
                                         .set_browse_name("LinkType")
                                         .set_display_name(u"Направление"),
                       .supertype_id = dev::DeviceType});

  // Mode enumeration data type with its EnumStrings array.
  factory.CreateNode(scada::NodeState{
      .node_id = dev::Iec60870ModeDataType,
      .node_class = scada::NodeClass::DataType,
      .parent_id = {scada::id::Enumeration, scada::NamespaceIndexes::NS0},
      .reference_type_id = {scada::id::HasSubtype,
                            scada::NamespaceIndexes::NS0},
      .attributes = scada::NodeAttributes{}
                        .set_browse_name("Iec60870ModeType")
                        .set_display_name(u"Режим МЭК-60870"),
      .supertype_id = {scada::id::Enumeration, scada::NamespaceIndexes::NS0}});
  factory.CreateNode(scada::NodeState{
      .node_id = dev::Iec60870ModeDataType_EnumStrings,
      .node_class = scada::NodeClass::Variable,
      .type_definition_id = {scada::id::PropertyType,
                             scada::NamespaceIndexes::NS0},
      .parent_id = dev::Iec60870ModeDataType,
      .reference_type_id = {scada::id::HasProperty,
                            scada::NamespaceIndexes::NS0},
      .attributes =
          scada::NodeAttributes{}
              .set_browse_name("EnumStrings")
              .set_display_name(u"EnumStrings")
              .set_data_type(
                  {scada::id::LocalizedText, scada::NamespaceIndexes::NS0})
              .set_value(scada::Variant{std::vector<scada::LocalizedText>{
                  u"Polling", u"Retransmission", u"Listening"}})});

  // Iec60870LinkType : LinkType, with its Mode property declaration.
  factory.CreateNode(scada::NodeState{
      .node_id = dev::Iec60870LinkType,
      .node_class = scada::NodeClass::ObjectType,
      .parent_id = dev::LinkType,
      .reference_type_id = {scada::id::HasSubtype,
                            scada::NamespaceIndexes::NS0},
      .attributes = scada::NodeAttributes{}
                        .set_browse_name("Iec60870LinkType")
                        .set_display_name(u"Направление МЭК-60870"),
      .supertype_id = dev::LinkType});
  factory.CreateNode(scada::NodeState{
      .node_id = dev::Iec60870LinkType_Mode,
      .node_class = scada::NodeClass::Variable,
      .type_definition_id = {scada::id::PropertyType,
                             scada::NamespaceIndexes::NS0},
      .parent_id = dev::Iec60870LinkType,
      .reference_type_id = {scada::id::HasProperty,
                            scada::NamespaceIndexes::NS0},
      .attributes = scada::NodeAttributes{}
                        .set_browse_name("Mode")
                        .set_display_name(u"Режим")
                        .set_data_type(dev::Iec60870ModeDataType)});

  // Iec60870DeviceType : DeviceType.
  factory.CreateNode(scada::NodeState{
      .node_id = dev::Iec60870DeviceType,
      .node_class = scada::NodeClass::ObjectType,
      .parent_id = dev::DeviceType,
      .reference_type_id = {scada::id::HasSubtype,
                            scada::NamespaceIndexes::NS0},
      .attributes = scada::NodeAttributes{}
                        .set_browse_name("Iec60870DeviceType")
                        .set_display_name(u"Устройство МЭК-60870"),
      .supertype_id = dev::DeviceType});

  // HasDevice reference type (GenericNodeFactory cannot create ReferenceType).
  address_space.AddStaticNode<scada::ReferenceType>(
      scada::data_items::id::HasDevice, "HasDevice");
}

}  // namespace

class PropertyDefsTest : public Test {
 protected:
  PropertyDefsTest();

  NodeRef CreateDataItem(std::string_view channel_path);

  scada_test::ScadaTestAddressSpace address_space;
  GenericNodeFactory node_factory{address_space};

  // StaticNodeService is a *snapshot* of the address space, not a live view, so
  // anything created after it is built stays invisible. This fixture (and
  // CreateDataItem) add nodes after construction, so re-sync before each lookup.
  // Re-syncing is cheap and idempotent: Add() uses try_emplace, so already
  // known nodes are skipped and only new ones are picked up.
  std::shared_ptr<StaticNodeService> node_service =
      std::make_shared<StaticNodeService>();

  void SyncNodeService() {
    node_service->AddAll(scada::MakeNodeStates(address_space));
  }

  TestExecutor executor;
  StrictMock<MockTaskManager> task_manager;
  StrictMock<MockDialogService> dialog_service;
  PropertyContext property_context{executor, *node_service, task_manager,
                                   dialog_service};

  ChannelPropertyDefinition channel_property_definition{u"Title", true};

  inline static const scada::NodeId data_item_id{1, 1};
  inline static const scada::NodeId data_group_id{2, 1};
  inline static const scada::NodeId link_id{
      1, scada::NamespaceIndexes::IEC60870_LINK};
  inline static const scada::NodeId device_id{
      1, scada::NamespaceIndexes::IEC60870_DEVICE};
  inline static const char16_t kLinkDisplayName[] = u"LinkDisplayName";
  inline static const char16_t kDeviceDisplayName[] = u"DeviceDisplayName";
};

PropertyDefsTest::PropertyDefsTest() {
  AddIec60870TestTypes(address_space);

  // Create Link.
  node_factory.CreateNode(
      scada::NodeState{}
          .set_node_id(link_id)
          .set_node_class(scada::NodeClass::Object)
          .set_type_definition_id(scada::devices::id::Iec60870LinkType)
          .set_parent(scada::id::Organizes, scada::devices::id::Devices)
          .set_attributes(
              scada::NodeAttributes{}.set_display_name(kLinkDisplayName))
          .set_properties(scada::NodeProperties{
              {scada::devices::id::Iec60870LinkType_Mode, scada::Variant{0}}}));

  // Create Device.
  node_factory.CreateNode(
      scada::NodeState{}
          .set_node_id(device_id)
          .set_node_class(scada::NodeClass::Object)
          .set_type_definition_id(scada::devices::id::Iec60870DeviceType)
          .set_parent(scada::id::Organizes, link_id)
          .set_attributes(
              scada::NodeAttributes{}.set_display_name(kDeviceDisplayName)));

  // Create Data Group.
  {
    auto [status, node_ptr] = node_factory.CreateNode(
        scada::NodeState{}
            .set_node_id(data_group_id)
            .set_node_class(scada::NodeClass::Object)
            .set_type_definition_id(scada::data_items::id::DataGroupType)
            .set_parent(scada::id::Organizes,
                        scada::data_items::id::DataItems));
    scada::base::Check(status);
    scada::base::Check(node_ptr);

    scada::AddReference(address_space, scada::data_items::id::HasDevice,
                        data_group_id, device_id);
  }

  SyncNodeService();
}

NodeRef PropertyDefsTest::CreateDataItem(std::string_view channel_path) {
  auto [status, node_ptr] = node_factory.CreateNode(
      scada::NodeState{}
          .set_node_id(data_item_id)
          .set_node_class(scada::NodeClass::Variable)
          .set_type_definition_id(scada::data_items::id::DiscreteItemType)
          .set_parent(scada::id::Organizes, data_group_id));
  scada::base::Check(status);
  scada::base::Check(node_ptr);

  scada::SetPropertyValue(*node_ptr, scada::data_items::id::DataItemType_Input1,
                          scada::String{channel_path});

  SyncNodeService();
  return node_service->GetNode(data_item_id);
}

TEST_F(PropertyDefsTest, GetText_Device) {
  auto data_item_node = CreateDataItem(
      MakeNodeIdFormula(MakeNestedNodeId(device_id, "device.channel.path")));

  EXPECT_EQ(u16format(L"{} : {}", kLinkDisplayName, kDeviceDisplayName),
            channel_property_definition.GetText(
                property_context, data_item_node,
                scada::data_items::id::DataItemType_Input1));
}

TEST_F(PropertyDefsTest, GetText_GroupDevice) {
  auto data_item_node = CreateDataItem("GROUP_DEVICE!device.channel.path");

  EXPECT_EQ(ChannelPropertyDefinition::kParentGroupDevice,
            channel_property_definition.GetText(
                property_context, data_item_node,
                scada::data_items::id::DataItemType_Input1));
}

TEST_F(PropertyDefsTest, Enum) {
  auto mode_prop_def =
      node_service->GetNode(scada::devices::id::Iec60870LinkType_Mode);
  auto mode_data_type = mode_prop_def.data_type();
  ASSERT_TRUE(IsSubtypeOf(mode_data_type, scada::id::Enumeration));

  auto link = node_service->GetNode(link_id);
  auto* prop_def = PropertyService{}.GetPropertyDef(mode_prop_def);
  ASSERT_TRUE(prop_def);
  EXPECT_EQ(u"Polling",
            prop_def->GetText(property_context, link, mode_prop_def.node_id()));
}

TEST_F(PropertyDefsTest, GetChildPropertyDefsAsync_ReturnsChildTypeProperties) {
  CreateDataItem("GROUP_DEVICE!device.channel.path");

  PropertyService property_service;
  TestExecutor executor;
  auto property_defs = WaitAwaitable(
      executor, property_service.GetChildPropertyDefsAsync(
                    executor,
                    node_service->GetNode(data_group_id)));

  const auto has_input1 =
      std::ranges::any_of(property_defs, [](const auto& property_def) {
        return property_def.first.node_id() ==
               scada::data_items::id::DataItemType_Input1;
      });
  EXPECT_TRUE(has_input1);
}

TEST_F(PropertyDefsTest, DeviceChoiceHandler_LoadsChoicesFromCoroutine) {
  CreateDataItem("GROUP_DEVICE!device.channel.path");

  auto editor = channel_property_definition.GetPropertyEditor(
      property_context, node_service->GetNode(data_item_id),
      scada::data_items::id::DataItemType_Input1);
  ASSERT_TRUE(editor.async_choice_handler);

  std::vector<std::u16string> choices;
  bool completed = false;
  editor.async_choice_handler(
      [&](const std::vector<std::u16string>& batch, bool last) {
        choices.insert(choices.end(), batch.begin(), batch.end());
        completed = last;
      });
  Drain(executor);

  EXPECT_TRUE(completed);
  EXPECT_THAT(choices,
              IsSupersetOf({std::u16string{kChoiceNone},
                            u16format(L"{} : {}", kLinkDisplayName,
                                      kDeviceDisplayName)}));
}
