#pragma once

#include "base/promise.h"
#include "export/configuration/export_data.h"

class NodeService;

class ExportDataBuilder {
 public:
  explicit ExportDataBuilder(NodeService& node_service)
      : node_service_{node_service} {}

  promise<ExportData> Build() const;

 private:
  std::vector<ExportData::Property> CollectProps() const;

  NodeService& node_service_;
};
