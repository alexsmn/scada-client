#pragma once

#include "base/promise.h"

#include <filesystem>

class NodeService;
class DialogService;
class TaskManager;
struct ExportData;

class ExportConfigurationCommand {
 public:
  promise<> Execute() const;

  NodeService& node_service_;
  DialogService& dialog_service_;

 private:
  promise<ExportData> CollectExportData() const;
  void SaveExportData(const ExportData& export_data,
                      std::ostream& stream) const;
  promise<void> ExportTo(const std::filesystem::path& path) const;
};

class ImportConfigurationCommand {
 public:
  promise<void> Execute() const;

  NodeService& node_service_;
  TaskManager& task_manager_;
  DialogService& dialog_service_;

 private:
  promise<void> ImportFrom(const std::filesystem::path& path) const;
};
