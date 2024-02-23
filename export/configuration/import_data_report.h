#pragma once

class NodeService;
struct DiffData;
struct ImportData;

void ShowImportReport(const ImportData& import_data, NodeService& node_service);

void ShowDiffReport(const DiffData& diff_data, NodeService& node_service);

class ScopedImportReportSuppressor {
 public:
  ScopedImportReportSuppressor();
  ~ScopedImportReportSuppressor();
};