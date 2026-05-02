#pragma once

#include "base/awaitable.h"
#include "base/promise.h"

#include <filesystem>
#include <memory>

class Executor;
class NodeService;
class DialogService;
class TaskManager;
struct ExportData;
struct ImportData;

class ExportConfigurationCommand {
 public:
  promise<void> Execute(DialogService& dialog_service) const;

  NodeService& node_service_;
  std::shared_ptr<Executor> executor_;

 private:
  promise<ExportData> CollectExportData() const;

  void SaveExportData(const ExportData& export_data,
                      std::ostream& stream) const;

  Awaitable<void> ExecuteAsync(DialogService& dialog_service) const;

  promise<void> ExportTo(const std::filesystem::path& path,
                         DialogService& dialog_service) const;

  Awaitable<void> ExportToAsync(const std::filesystem::path& path,
                                DialogService& dialog_service) const;
};

class ImportConfigurationCommand {
 public:
  promise<void> Execute(DialogService& dialog_service) const;

  NodeService& node_service_;
  TaskManager& task_manager_;
  std::shared_ptr<Executor> executor_;

 private:
  promise<void> ImportFrom(const std::filesystem::path& path,
                           DialogService& dialog_service) const;

  Awaitable<void> ExecuteAsync(DialogService& dialog_service) const;

  Awaitable<void> ImportFromAsync(const std::filesystem::path& path,
                                  DialogService& dialog_service) const;

  ExportData LoadExportData(const std::filesystem::path& path) const;
};
