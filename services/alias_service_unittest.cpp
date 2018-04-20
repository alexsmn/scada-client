#include "services/alias_service.h"

#include "common/address_space/test/address_space_node_service_test_context.h"
#include "common/scada_node_ids.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

scada::NodeState& CreateDataItem(TestAddressSpace& address_space,
                                 const scada::NodeId& node_id) {
  auto& node_state = address_space.nodes.emplace_back(
      scada::NodeState{node_id, scada::NodeClass::Variable, id::AnalogItemType,
                       id::DataItems, scada::id::Organizes});
  return node_state;
}

}  // namespace

TEST(AliasService, Init) {
  AddressSpaceNodeServiceTestContext context;

  const scada::NodeId kDataItem1{1, 100};
  auto& data_item1 = CreateDataItem(context.server_address_space, kDataItem1);
  const char kAlias[] = "TestAlias";
  SetProperty(data_item1, id::DataItemType_Alias, kAlias);

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
  SetProperty(data_item1, id::DataItemType_Alias, kAlias);

  AliasService alias_service{AliasServiceContext{
      std::make_shared<NullLogger>(),
      context.node_service,
  }};

  const char kAlias2[] = "TestAlias2";
  SetProperty(data_item1, id::DataItemType_Alias, kAlias2);
  context.server_address_space.NotifyNodeSemanticsChanged(kDataItem1);

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
  SetProperty(data_item1, id::DataItemType_Alias, kAlias);

  AliasService alias_service{AliasServiceContext{
      std::make_shared<NullLogger>(),
      context.node_service,
  }};

  const char kAlias2[] = "TestAlias2";
  SetProperty(data_item1, id::DataItemType_Alias, kAlias2);
  context.server_address_space.NotifyNodeSemanticsChanged(kDataItem1);

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
  SetProperty(data_item1, id::DataItemType_Alias, kAlias);
  context.server_address_space.NotifyModelChanged(
      {data_item1.node_id, data_item1.type_definition_id,
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
    SetProperty(data_item1, id::DataItemType_Alias, kAlias);
    context.server_address_space.NotifyModelChanged(
        {data_item1.node_id, data_item1.type_definition_id,
         scada::ModelChangeEvent::NodeAdded});
  }

  context.server_address_space.DeleteNode(kDataItem1);
  context.server_address_space.NotifyModelChanged(
      {kDataItem1, {}, scada::ModelChangeEvent::NodeDeleted});

  alias_service.Resolve(
      kAlias, [&](scada::Status&& status, const scada::NodeId& node_id) {
        EXPECT_FALSE(status);
      });
}
