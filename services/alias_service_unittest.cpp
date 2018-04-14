#include "services/alias_service.h"

#include "common/address_space/test/address_space_node_service_test_context.h"

#include <gmock/gmock.h>

TEST(AliasService, Init) {
  AddressSpaceNodeServiceTestContext context;

  const char kAlias[] = "TestAlias";
  context.data_item1.SetPropertyValue(id::DataItemType_Alias, kAlias);

  AliasService alias_service{AliasServiceContext{
      std::make_shared<NullLogger>(),
      context.node_service,
  }};

  const char kUnknownAlias[] = "UknownAlias";

  alias_service.Resolve(
      kAlias, [&](scada::Status&& status, const scada::NodeId& node_id) {
        EXPECT_TRUE(status);
        EXPECT_EQ(node_id, context.data_item1.id());
      });

  alias_service.Resolve(
      kUnknownAlias, [&](scada::Status&& status, const scada::NodeId& node_id) {
        EXPECT_FALSE(status);
      });
}

TEST(AliasService, ModifyAlias) {
  AddressSpaceNodeServiceTestContext context;

  const char kAlias[] = "TestAlias";
  context.data_item1.SetPropertyValue(id::DataItemType_Alias, kAlias);

  AliasService alias_service{AliasServiceContext{
      std::make_shared<NullLogger>(),
      context.node_service,
  }};

  const char kAlias2[] = "TestAlias2";
  context.data_item1.SetPropertyValue(id::DataItemType_Alias, kAlias2);
  context.address_space.NotifyNodeModified(context.data_item1,
                                           {id::DataItemType_Alias});

  alias_service.Resolve(
      kAlias, [&](scada::Status&& status, const scada::NodeId& node_id) {
        EXPECT_FALSE(status);
      });

  alias_service.Resolve(
      kAlias2, [&](scada::Status&& status, const scada::NodeId& node_id) {
        EXPECT_TRUE(status);
        EXPECT_EQ(node_id, context.data_item1.id());
      });
}

TEST(AliasService, ClearAlias) {
  AddressSpaceNodeServiceTestContext context;

  const char kAlias[] = "TestAlias";
  context.data_item1.SetPropertyValue(id::DataItemType_Alias, kAlias);

  AliasService alias_service{AliasServiceContext{
      std::make_shared<NullLogger>(),
      context.node_service,
  }};

  context.data_item1.SetPropertyValue(id::DataItemType_Alias, {});
  context.address_space.NotifyNodeModified(context.data_item1,
                                           {id::DataItemType_Alias});

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

  const char kAlias[] = "TestAlias";

  auto& new_data_item =
      context.CreateDataItem(scada::NodeId{123, 12}, L"NewDataItem");
  new_data_item.SetPropertyValue(id::DataItemType_Alias, kAlias);
  context.address_space.NotifyNodeAdded(new_data_item);

  alias_service.Resolve(
      kAlias, [&](scada::Status&& status, const scada::NodeId& node_id) {
        EXPECT_TRUE(status);
        EXPECT_EQ(node_id, new_data_item.id());
      });
}

TEST(AliasService, DeleteDataItem) {
  AddressSpaceNodeServiceTestContext context;

  AliasService alias_service{AliasServiceContext{
      std::make_shared<NullLogger>(),
      context.node_service,
  }};

  const char kAlias[] = "TestAlias";
  const scada::NodeId kNewDataItemId{123, 12};

  {
    auto& new_data_item =
        context.CreateDataItem(kNewDataItemId, L"NewDataItem");
    new_data_item.SetPropertyValue(id::DataItemType_Alias, kAlias);
    context.address_space.NotifyNodeAdded(new_data_item);
  }

  context.address_space.RemoveNode(kNewDataItemId);

  alias_service.Resolve(
      kAlias, [&](scada::Status&& status, const scada::NodeId& node_id) {
        EXPECT_FALSE(status);
      });
}
