#pragma once

#include "base/promise.h"

#include <filesystem>

class NodeService;
class DialogService;
class TaskManager;
struct ExportData;
struct ImportData;

class ExportConfigurationCommand {
 public:
  promise<void> Execute(DialogService& dialog_service) const;

  NodeService& node_service_;

 private:
  promise<ExportData> CollectExportData() const;

  void SaveExportData(const ExportData& export_data,
                      std::ostream& stream) const;

  promise<void> ExportTo(const std::filesystem::path& path,
                         DialogService& dialog_service) const;
};

class ImportConfigurationCommand {
 public:
  promise<void> Execute(DialogService& dialog_service) const;

  NodeService& node_service_;
  TaskManager& task_manager_;

 private:
  promise<void> ImportFrom(const std::filesystem::path& path,
                           DialogService& dialog_service) const;
};
