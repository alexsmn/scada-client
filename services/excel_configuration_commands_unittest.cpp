#include "services/import_export.h"

#include "base/table_reader.h"
#include "base/table_writer.h"
#include "common/test/address_space_node_service_test_context.h"

#include <gmock/gmock.h>
#include <sstream>

TEST(ExcelConfigurationCommands, ExportImport) {
  AddressSpaceNodeServiceTestContext context;

  std::wstringstream stream;
  TableWriter writer{stream};
  ExportConfiguration(context.node_service, writer);

  context.data_item1.SetDisplayName(L"DataItem1 renamed");

  TableReader reader{stream};
  auto import_data = ImportConfiguration(context.node_service, reader);
}
