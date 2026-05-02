#pragma once

#include "base/awaitable.h"
#include "base/any_executor.h"
#include "base/promise.h"
#include "export/configuration/export_data.h"

#include <memory>

class Executor;
class NodeService;

class ExportDataBuilder {
 public:
  ExportDataBuilder(NodeService& node_service,
                    std::shared_ptr<Executor> executor)
      : node_service_{node_service}, executor_{std::move(executor)} {}

  promise<ExportData> Build() const;
  Awaitable<ExportData> BuildAsync(AnyExecutor executor) const;

 private:
  std::vector<ExportData::Property> CollectProps() const;

  NodeService& node_service_;
  std::shared_ptr<Executor> executor_;
};
