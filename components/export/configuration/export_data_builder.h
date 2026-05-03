#pragma once

#include "base/awaitable.h"
#include "base/any_executor.h"
#include "export/configuration/export_data.h"

class NodeService;

class ExportDataBuilder {
 public:
  ExportDataBuilder(NodeService& node_service,
                    AnyExecutor executor)
      : node_service_{node_service}, executor_{std::move(executor)} {}

  Awaitable<ExportData> BuildAsync(AnyExecutor executor) const;

 private:
  std::vector<ExportData::Property> CollectProps() const;

  NodeService& node_service_;
  AnyExecutor executor_;
};
