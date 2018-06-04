#include "services/alias_service.h"

#include "common/address_space/test/address_space_node_service_test_context.h"
#include "common/scada_node_ids.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

scada::Variable& CreateDataItem(TestAddressSpace& address_space,
                                const scada::NodeId& node_id) {
  auto* type_definition =
      scada::AsVariableType(address_space.GetNode(id::AnalogItemType));
  assert(type_definition);

  auto& node = address_space.AddStaticNode<scada::GenericVariable>(
      node_id, scada::QualifiedName{}, scada::LocalizedText{},
      type_definition->data_type(), scada::Variant{});

  scada::AddReference(address_space, scada::id::HasTypeDefinition, node,
                      *type_definition);
  scada::AddReference(address_space, scada::id::Organizes, id::DataItems,
                      node_id);

  return node;
}

}  // namespace

TEST(AliasService, Init) {
  AddressSpaceNodeServiceTestContext context;

  const scada::NodeId kDataItem1{1, 100};
  auto& data_item1 = CreateDataItem(context.server_address_space, kDataItem1);
  const char kAlias[] = "TestAlias";
  data_item1.SetPropertyValue(id::DataItemType_Alias, kAlias);

  AliasService alias_service{AliasServiceContext{
      std::make_shared<NullLogger>(),
      context.node_service,
  }};

  const char kUnknownAlias[] = "UknownAlias";

  alias_service.Resolve(
      kAlias, [&](scada::Status&& status, const scada::NodeId& node_id) {
        EXPECT_TRUE(status);
        EXPECT_EQ(node_id, kDataItem1);
      });

  alias_service.Resolve(
      kUnknownAlias, [&](scada::Status&& status, const scada::NodeId& node_id) {
        EXPECT_FALSE(status);
      });
}

TEST(AliasService, ModifyAlias) {
  AddressSpaceNodeServiceTestContext context;

  const scada::NodeId kDataItem1{1, 100};
  auto& data_item1 = CreateDataItem(context.server_address_space, kDataItem1);
  const char kAlias[] = "TestAlias";
  data_item1.SetPropertyValue(id::DataItemType_Alias, kAlias);

  AliasService alias_service{AliasServiceContext{
      std::make_shared<NullLogger>(),
      context.node_service,
  }};

  const char kAlias2[] = "TestAlias2";
  data_item1.SetPropertyValue(id::DataItemType_Alias, kAlias2);
  context.server_address_space.NotifySemanticChanged(
      kDataItem1, scada::GetTypeDefinitionId(data_item1));

  alias_service.Resolve(
      kAlias, [&](scada::Status&& status, const scada::NodeId& node_id) {
        EXPECT_FALSE(status);
      });

  alias_service.Resolve(
      kAlias2, [&](scada::Status&& status, const scada::NodeId& node_id) {
        EXPECT_TRUE(status);
        EXPECT_EQ(node_id, kDataItem1);
      });
}

TEST(AliasService, ClearAlias) {
  AddressSpaceNodeServiceTestContext context;

  const scada::NodeId kDataItem1{1, 100};
  auto& data_item1 = CreateDataItem(context.server_address_space, kDataItem1);
  const char kAlias[] = "TestAlias";
  data_item1.SetPropertyValue(id::DataItemType_Alias, kAlias);

  AliasService alias_service{AliasServiceContext{
      std::make_shared<NullLogger>(),
      context.node_service,
  }};

  const char kAlias2[] = "TestAlias2";
  data_item1.SetPropertyValue(id::DataItemType_Alias, kAlias2);
  context.server_address_space.NotifySemanticChanged(
      kDataItem1, scada::GetTypeDefinitionId(data_item1));

  alias_service.Resolve(
      kAlias, [&](scada::Status&& status, const scada::NodeId& node_id) {
        EXPECT_FALSE(status);
      });
}

TEST(AliasService, CreateDataItem) {
  AddressSpaceNodeServiceTestContext context;

  AliasService alias_service{AliasServiceContext{
      std::make_shared<NullLogger>(),
      context.node_service,
  }};

  const scada::NodeId kDataItem1{1, 100};
  auto& data_item1 = CreateDataItem(context.server_address_space, kDataItem1);
  const char kAlias[] = "TestAlias";
  data_item1.SetPropertyValue(id::DataItemType_Alias, kAlias);
  context.server_address_space.NotifyModelChanged(
      {data_item1.id(), scada::GetTypeDefinitionId(data_item1),
       scada::ModelChangeEvent::NodeAdded});

  alias_service.Resolve(
      kAlias, [&](scada::Status&& status, const scada::NodeId& node_id) {
        EXPECT_TRUE(status);
        EXPECT_EQ(node_id, kDataItem1);
      });
}

TEST(AliasService, DeleteDataItem) {
  AddressSpaceNodeServiceTestContext context;

  AliasService alias_service{AliasServiceContext{
      std::make_shared<NullLogger>(),
      context.node_service,
  }};

  const scada::NodeId kDataItem1{1, 100};
  const char kAlias[] = "TestAlias";

  {
    auto& data_item1 = CreateDataItem(context.server_address_space, kDataItem1);
    data_item1.SetPropertyValue(id::DataItemType_Alias, kAlias);
    context.server_address_space.NotifyModelChanged(
        {data_item1.id(), scada::GetTypeDefinitionId(data_item1),
         scada::ModelChangeEvent::NodeAdded});
  }

  context.server_address_space.RemoveNode(kDataItem1);
  context.server_address_space.NotifyModelChanged(
      {kDataItem1, {}, scada::ModelChangeEvent::NodeDeleted});

  alias_service.Resolve(
      kAlias, [&](scada::Status&& status, const scada::NodeId& node_id) {
        EXPECT_FALSE(status);
      });
}
