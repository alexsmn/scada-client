#include "services/import_export.h"

#include "base/table_reader.h"
#include "base/table_writer.h"
#include "common/address_space/test/address_space_node_service_test_context.h"

#include <gmock/gmock.h>
#include <sstream>

using namespace testing;

TEST(ExcelConfigurationCommands, ExportImport) {
  AddressSpaceNodeServiceTestContext context;

  std::wstringstream stream;
  TableWriter writer{stream};
  ExportConfiguration(context.node_service, writer);

  auto* node = context.server_address_space.GetNode(
      context.server_address_space.kTestNode1Id);
  ASSERT_NE(node, nullptr);
  node->SetDisplayName(L"Renamed node");
  context.server_address_space.NotifySemanticChanged(
      node->id(), scada::GetTypeDefinitionId(*node));

  TableReader reader{stream};
  auto import_data = ImportConfiguration(context.node_service, reader);
}
