#pragma once

#include "base/any_executor.h"

#include "base/awaitable.h"

#include <filesystem>
#include <memory>

class NodeService;
class DialogService;
class TaskManager;
struct ExportData;
struct ImportData;

class ExportConfigurationCommand {
 public:
  Awaitable<void> Execute(DialogService& dialog_service) const;

  NodeService& node_service_;
  AnyExecutor executor_;

 private:
  Awaitable<ExportData> CollectExportData() const;

  void SaveExportData(const ExportData& export_data,
                      std::ostream& stream) const;

  Awaitable<void> ExportTo(const std::filesystem::path& path,
                           DialogService& dialog_service) const;
};

class ImportConfigurationCommand {
 public:
  Awaitable<void> Execute(DialogService& dialog_service) const;

  NodeService& node_service_;
  TaskManager& task_manager_;
  AnyExecutor executor_;

 private:
  Awaitable<void> ImportFrom(const std::filesystem::path& path,
                             DialogService& dialog_service) const;

  ExportData LoadExportData(const std::filesystem::path& path) const;
};
