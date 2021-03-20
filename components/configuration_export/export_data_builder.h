#pragma once

#include "base/promise.h"
#include "components/configuration_export/export_data.h"

class NodeService;

class ExportDataBuilder {
 public:
  explicit ExportDataBuilder(NodeService& node_service)
      : node_service_{node_service} {}

  promise<ExportData> Build();

 private:
  NodeService& node_service_;
};
